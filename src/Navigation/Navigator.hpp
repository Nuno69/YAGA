#include <algorithm>
#include <cctype>
#include <chrono>
#include <functional>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

namespace GOTHIC_NAMESPACE
{
// Route planning and auto-walk.
//
// The graph is the game's own way net: the waypoints and ways NPCs walk on. The search is done here (Dijkstra over the
// way net) instead of with zCWayNet::FindRoute, because a recalculation has to leave out the legs the hero already
// failed to walk, and the engine search cannot be told to do that.
//
// Walking hands the hero to the engine's NPC movement (oCMsgMovement EV_GOTOPOS), whose "robust trace" turns around
// small obstacles. Where that movement cannot go (swimming, or a ledge it gives up on), the navigator drives the hero
// itself: it faces the waypoint and holds the forward key, with a jump when the hero stops moving. It also opens
// closed doors in the way, notices when the hero is stuck or pushed off the route, and then recalculates around the
// blocked leg.
namespace Navigator
{
constexpr float ArriveRadius = 150.0f;       // a waypoint counts as reached within this XZ distance (cm)
constexpr float EngineDoneRadius = 400.0f;   // ... or within this one when the engine reports the move as finished
constexpr float StartSearchRadius = 5000.0f; // waypoints considered as the first step of a route
constexpr float StuckWindowMs = 2500.0f;     // no progress for this long counts as stuck
constexpr float StuckMinMove = 40.0f;        // ... where progress means moving at least this far
constexpr float OffRouteDistance = 1500.0f;  // farther than this from the current leg triggers a recalculation
constexpr float DoorSearchRadius = 600.0f;   // closed doors this close to the hero are opened when a step fails
constexpr float DoorPressMs = 500.0f;        // how long action + forward is held to open a door
constexpr float DoorTimeoutMs = 3000.0f;     // a door that is not open by then is given up on
constexpr float DriveJumpAfterMs = 1200.0f;  // driving without progress this long: try a jump / climb
constexpr float DriveJumpHoldMs = 300.0f;
constexpr float DriveGiveUpMs = 4000.0f;     // driving without progress this long: the leg is blocked
// Water deeper than this makes the hero swim. Legs through it are left out of routes: NPC movement cannot swim, and
// some lakes kill a swimming hero outright (the one below the intro cliff does).
constexpr float DeepWater = 100.0f;
constexpr int MaxWaterReplans = 16;
constexpr int MaxRetriesPerWaypoint = 1;     // re-issue the same step before giving up on the leg
constexpr int MaxRecalculations = 8;         // per navigation, before giving up
constexpr int MaxStartCandidates = 16;       // nearest waypoints checked for a clear line from the hero

enum class StopReason
{
    Arrived,
    Cancelled,
    Attacked,
    NoRoute,
    GaveUp,
    WorldChanged,
    Dead
};

enum class Phase
{
    Walking,     // the engine's NPC movement walks the leg
    Driving,     // the navigator holds the forward key itself
    OpeningDoor
};

struct Plan
{
    std::vector<zCWaypoint *> Waypoints;
    zCWaypoint *Start = nullptr;
    zCWaypoint *Target = nullptr;
    float Length = 0.0f; // from the hero through every waypoint, in cm
    double Milliseconds = 0.0;
    int Reachable = 0; // waypoints the search reached
};

using Leg = std::pair<zCWaypoint *, zCWaypoint *>;

inline bool Active = false;
inline Phase CurrentPhase = Phase::Walking;
inline std::vector<zCWaypoint *> Path;
inline size_t Index = 0;
inline zCWaypoint *TargetWaypoint = nullptr;
inline bool HasTargetSector = false;
inline SECTOR TargetSector{};
inline zCWorld *NavWorld = nullptr;
inline zCWayNet *NavWayNet = nullptr;
inline zVEC3 StuckReference;
inline float StuckReferenceTime = 0.0f;
inline int RetriesOnWaypoint = 0;
inline int Recalculations = 0;
inline int LastHitpoints = 0;
inline int WaypointsSinceAnnouncement = 0;
inline float StartedAt = 0.0f;
inline float PhaseStartedAt = 0.0f;
inline std::set<Leg> BlockedLegs;               // legs the hero failed to walk during this navigation
inline std::set<zCWaypoint *> BlockedStarts;    // waypoints the hero failed to reach from where it stood
inline std::set<oCMobDoor *> TriedDoors;        // doors already opened (or found locked) during this navigation
inline float DriveJumpUntil = 0.0f;
inline bool DriveJumped = false;
inline oCMobDoor *DoorBeingOpened = nullptr;

// Logical keys the navigator is holding down (bit per GAME_* key), read by the zCInput_Win32::GetState hook, and the
// presses not yet reported by GetToggled.
inline unsigned int InjectedKeys = 0;
inline unsigned int InjectedToggles = 0;
// The unhooked GetState, so real key presses can be told apart from injected ones.
inline std::function<float(unsigned short)> RealKeyState;

// ---------------------------------------------------------------- helpers

bool IsKeyInjected(const unsigned short logicalKey)
{
    return logicalKey < 32 && (InjectedKeys & (1u << logicalKey)) != 0;
}

void Inject(const unsigned short logicalKey, const bool down)
{
    const unsigned int bit = 1u << logicalKey;
    if (down)
    {
        if (!(InjectedKeys & bit))
            InjectedToggles |= bit;
        InjectedKeys |= bit;
    }
    else
        InjectedKeys &= ~bit;
}

void ReleaseKeys()
{
    InjectedKeys = 0;
    InjectedToggles = 0;
}

// One injected press is reported once, like a real key going down.
bool TakeInjectedToggle(const unsigned short logicalKey)
{
    const unsigned int bit = logicalKey < 32 ? 1u << logicalKey : 0u;
    if (!(InjectedToggles & bit))
        return false;
    InjectedToggles &= ~bit;
    return true;
}

float RealState(const unsigned short logicalKey)
{
    return RealKeyState ? RealKeyState(logicalKey) : zinput->GetState(logicalKey);
}

bool HeroInWater()
{
    return player->anictrl && player->anictrl->IsInWater();
}

float NowMs()
{
    return ztimer ? ztimer->totalTimeFloat : 0.0f;
}

zCWayNet *GetWayNet()
{
    oCWorld *world = ogame ? ogame->GetGameWorld() : nullptr;
    return world ? world->wayNet : nullptr;
}

std::string WaypointName(zCWaypoint *wp)
{
    if (!wp)
        return "none";
    return std::string(wp->GetName().ToChar());
}

int Meters(const float centimeters)
{
    return static_cast<int>(centimeters / 100.0f + 0.5f);
}

zVEC3 HeroPosition()
{
    return player->GetPositionWorld();
}

Leg MakeLeg(zCWaypoint *a, zCWaypoint *b)
{
    return a < b ? Leg(a, b) : Leg(b, a);
}

bool IsLegBlocked(zCWaypoint *a, zCWaypoint *b)
{
    return BlockedLegs.count(MakeLeg(a, b)) != 0;
}

// Line of sight against the static world and collidable vobs, ignoring characters and see-through polygons
// (grass, leaves). The target is lifted to roughly hip height, where the hero's own position sits.
bool HasClearLine(const zVEC3 &from, const zVEC3 &waypointPosition)
{
    oCWorld *world = ogame->GetGameWorld();
    const zVEC3 to = waypointPosition + zVEC3(0.0f, 80.0f, 0.0f);
    const int flags = zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER |
                      zTRACERAY_POLY_IGNORE_TRANSP;
    return !world->TraceRayNearestHit(from, to - from, player, flags);
}

// Depth of the water at a spot (0 = dry): the water surface found by a downward trace that hits water polygons, minus
// the ground found by one that does not.
float WaterDepthAt(const zVEC3 &position)
{
    oCWorld *world = ogame->GetGameWorld();
    const zVEC3 origin = position + zVEC3(0.0f, 250.0f, 0.0f);
    const zVEC3 ray(0.0f, -1500.0f, 0.0f);

    if (!world->TraceRayNearestHit(origin, ray, player, zTRACERAY_STAT_POLY | zTRACERAY_POLY_TEST_WATER |
                                                            zTRACERAY_VOB_IGNORE))
        return 0.0f;

    zCPolygon *poly = world->traceRayReport.foundPoly;
    if (!poly || !poly->material || poly->material->matGroup != zMAT_GROUP_WATER)
        return 0.0f;

    const float surface = world->traceRayReport.foundIntersection[VY];
    if (!world->TraceRayNearestHit(origin, ray, player,
                                   zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE | zTRACERAY_POLY_IGNORE_TRANSP))
        return 1500.0f;

    return (std::max)(0.0f, surface - world->traceRayReport.foundIntersection[VY]);
}

bool CrossesDeepWater(const zVEC3 &from, const zVEC3 &to)
{
    for (const float t : {0.25f, 0.5f, 0.75f, 1.0f})
        if (WaterDepthAt(from + (to - from) * t) > DeepWater)
            return true;
    return false;
}

template <typename Fn> void ForEachWaypoint(zCWayNet *wayNet, Fn fn)
{
    for (auto node = wayNet->wplist.next; node; node = node->next)
        if (node->data)
            fn(node->data);
}

zCWaypoint *OtherEnd(zCWay *way, zCWaypoint *from)
{
    if (way->left == from)
        return way->right;
    if (way->right == from)
        return way->left;
    return nullptr;
}

// zCRoute keeps the start waypoint and the ways leading from it to the goal.
std::vector<zCWaypoint *> ExtractWaypoints(zCRoute *route)
{
    std::vector<zCWaypoint *> result;
    zCWaypoint *current = route->startwp;
    if (!current)
        return result;

    result.push_back(current);
    for (auto node = route->wayList.next; node; node = node->next)
    {
        zCWay *way = node->data;
        if (!way)
            continue;

        zCWaypoint *next = OtherEnd(way, current);
        if (!next)
            break;

        result.push_back(next);
        current = next;
    }

    return result;
}

float RemainingLength(const zVEC3 &from, const std::vector<zCWaypoint *> &waypoints, const size_t index)
{
    if (index >= waypoints.size())
        return 0.0f;

    float length = from.Distance(waypoints[index]->GetPositionWorld());
    for (size_t i = index + 1; i < waypoints.size(); ++i)
        length += waypoints[i - 1]->GetPositionWorld().Distance(waypoints[i]->GetPositionWorld());
    return length;
}

// Distance from a point to the segment a-b, measured in the XZ plane.
float DistanceToLegXZ(const zVEC3 &p, const zVEC3 &a, const zVEC3 &b)
{
    const float abx = b[VX] - a[VX];
    const float abz = b[VZ] - a[VZ];
    const float lengthSquared = abx * abx + abz * abz;
    float t = 0.0f;
    if (lengthSquared > 1.0f)
        t = std::clamp(((p[VX] - a[VX]) * abx + (p[VZ] - a[VZ]) * abz) / lengthSquared, 0.0f, 1.0f);

    const zVEC3 closest(a[VX] + abx * t, p[VY], a[VZ] + abz * t);
    return DistanceXZ(p, closest);
}

// ---------------------------------------------------------------- planning

// The first waypoint of a route: the nearest one the hero can see. The engine's own FindRoute(position, ...) takes
// the plain nearest waypoint, which is often on the other side of a wall or a cliff.
zCWaypoint *ChooseStartWaypoint(zCWayNet *wayNet, const zVEC3 &from)
{
    std::vector<std::pair<float, zCWaypoint *>> nearby;
    ForEachWaypoint(wayNet, [&](zCWaypoint *wp) {
        if (BlockedStarts.count(wp))
            return;
        const float distance = from.Distance(wp->GetPositionWorld());
        if (distance <= StartSearchRadius)
            nearby.emplace_back(distance, wp);
    });

    std::sort(nearby.begin(), nearby.end(),
              [](const auto &left, const auto &right) { return left.first < right.first; });

    const size_t checks = (std::min)(nearby.size(), static_cast<size_t>(MaxStartCandidates));
    for (size_t i = 0; i < checks; ++i)
        if (HasClearLine(from, nearby[i].second->GetPositionWorld()))
            return nearby[i].second;

    if (!nearby.empty())
        return nearby.front().second;

    return wayNet->GetNearestWaypoint(from);
}

std::vector<zCWaypoint *> WaypointsInSector(zCWayNet *wayNet, const SECTOR &sector)
{
    std::vector<zCWaypoint *> result;
    ForEachWaypoint(wayNet, [&](zCWaypoint *wp) {
        if (SameSector(SectorOf(wp->GetPositionWorld()), sector))
            result.push_back(wp);
    });

    const zVEC3 center = SectorCenter(sector);
    std::sort(result.begin(), result.end(), [&](zCWaypoint *left, zCWaypoint *right) {
        return DistanceXZ(left->GetPositionWorld(), center) < DistanceXZ(right->GetPositionWorld(), center);
    });
    return result;
}

// Dijkstra from one waypoint over the way net, leaving out blocked legs. The net has under 3000 waypoints, so a full
// search costs a fraction of a millisecond.
void ShortestPaths(zCWaypoint *start, std::unordered_map<zCWaypoint *, float> &distance,
                   std::unordered_map<zCWaypoint *, zCWaypoint *> &parent)
{
    using Item = std::pair<float, zCWaypoint *>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> open;

    distance[start] = 0.0f;
    open.push({0.0f, start});
    while (!open.empty())
    {
        const auto [cost, wp] = open.top();
        open.pop();
        if (cost > distance[wp])
            continue;

        for (auto node = wp->wayList.next; node; node = node->next)
        {
            zCWay *way = node->data;
            zCWaypoint *next = way ? OtherEnd(way, wp) : nullptr;
            if (!next || IsLegBlocked(wp, next))
                continue;

            const float nextCost = cost + wp->GetPositionWorld().Distance(next->GetPositionWorld());
            const auto found = distance.find(next);
            if (found == distance.end() || nextCost < found->second)
            {
                distance[next] = nextCost;
                parent[next] = wp;
                open.push({nextCost, next});
            }
        }
    }
}

// Skip leading waypoints that would make the hero walk back: while the following waypoint is in sight and no farther
// from the hero than from the waypoint before it, head straight for it.
void TrimLeadingWaypoints(std::vector<zCWaypoint *> &waypoints, const zVEC3 &from)
{
    while (waypoints.size() >= 2 && !BlockedStarts.count(waypoints[1]))
    {
        const zVEC3 first = waypoints[0]->GetPositionWorld();
        const zVEC3 second = waypoints[1]->GetPositionWorld();
        if (from.Distance(second) <= first.Distance(second) && HasClearLine(from, second))
            waypoints.erase(waypoints.begin());
        else
            break;
    }
}

// Goals are tried in order (for a sector: nearest to its centre first); the first one the search reached wins.
bool PlanRouteOnce(zCWayNet *wayNet, const zVEC3 &from, const std::vector<zCWaypoint *> &goals, Plan &plan)
{
    const auto started = std::chrono::high_resolution_clock::now();
    const auto elapsed = [&]() {
        return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - started).count();
    };

    plan = Plan();
    plan.Start = ChooseStartWaypoint(wayNet, from);
    if (!plan.Start)
        return false;

    std::unordered_map<zCWaypoint *, float> distance;
    std::unordered_map<zCWaypoint *, zCWaypoint *> parent;
    ShortestPaths(plan.Start, distance, parent);
    plan.Reachable = static_cast<int>(distance.size());

    for (zCWaypoint *goal : goals)
    {
        if (distance.count(goal))
        {
            plan.Target = goal;
            break;
        }
    }

    if (!plan.Target)
    {
        plan.Milliseconds = elapsed();
        return false;
    }

    for (zCWaypoint *wp = plan.Target; wp; wp = wp == plan.Start ? nullptr : parent[wp])
        plan.Waypoints.push_back(wp);
    std::reverse(plan.Waypoints.begin(), plan.Waypoints.end());

    TrimLeadingWaypoints(plan.Waypoints, from);
    plan.Length = RemainingLength(from, plan.Waypoints, 0);
    plan.Milliseconds = elapsed();
    return true;
}

// Plans, then checks the route for deep water and plans again without the wet legs. Only the legs of the found route
// are traced, so this costs a few traces per waypoint instead of tracing the whole way net. A hero who is already
// swimming may swim the first leg, to get out.
bool PlanRoute(zCWayNet *wayNet, const zVEC3 &from, const std::vector<zCWaypoint *> &goals, Plan &plan)
{
    const auto started = std::chrono::high_resolution_clock::now();
    const bool swimming = player->anictrl && player->anictrl->IsInWater();

    for (int attempt = 0; attempt < MaxWaterReplans; ++attempt)
    {
        if (!PlanRouteOnce(wayNet, from, goals, plan))
            break;

        bool dry = true;
        zVEC3 previous = from;
        for (size_t i = 0; i < plan.Waypoints.size() && dry; ++i)
        {
            const zVEC3 current = plan.Waypoints[i]->GetPositionWorld();
            if (!(i == 0 && swimming) && CrossesDeepWater(previous, current))
            {
                dry = false;
                if (i == 0)
                    BlockedStarts.insert(plan.Waypoints[0]);
                else
                    BlockedLegs.insert(MakeLeg(plan.Waypoints[i - 1], plan.Waypoints[i]));
                YagaLog::Write(std::format("[nav] deep water on the way to {}, leaving that leg out",
                                           WaypointName(plan.Waypoints[i])));
            }
            previous = current;
        }

        if (dry)
        {
            plan.Milliseconds =
                std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - started).count();
            return true;
        }
    }

    plan.Waypoints.clear();
    plan.Milliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - started).count();
    return false;
}

void LogPlan(const char *what, const Plan &plan)
{
    std::string names;
    for (size_t i = 0; i < plan.Waypoints.size(); ++i)
    {
        if (i)
            names += " > ";
        names += WaypointName(plan.Waypoints[i]);
    }

    YagaLog::Write(std::format("[nav] {}: start {}, goal {}, {} waypoints, {} m, {} reachable, {} blocked legs, "
                               "{:.3f} ms",
                               what, WaypointName(plan.Start), WaypointName(plan.Target), plan.Waypoints.size(),
                               Meters(plan.Length), plan.Reachable, BlockedLegs.size(), plan.Milliseconds));
    YagaLog::Write("[nav] path: " + names);
}

// ---------------------------------------------------------------- walking

bool IsOurMessage(zCEventMessage *message)
{
    if (oCMsgMovement *movement = zDYNAMIC_CAST<oCMsgMovement>(message))
        return movement->subType == oCMsgMovement::EV_GOTOPOS || movement->subType == oCMsgMovement::EV_GOROUTE;

    return false;
}

bool HasPendingMessage()
{
    zCEventManager *em = player->GetEM();
    for (int i = 0, count = em->GetNumMessages(); i < count; ++i)
    {
        zCEventMessage *message = em->GetEventMessage(i);
        if (IsOurMessage(message) && !static_cast<oCNpcMessage *>(message)->deleted)
            return true;
    }
    return false;
}

void ClearMovement()
{
    zCEventManager *em = player->GetEM();
    for (int i = 0, count = em->GetNumMessages(); i < count; ++i)
    {
        zCEventMessage *message = em->GetEventMessage(i);
        if (zDYNAMIC_CAST<oCMsgMovement>(message) && IsOurMessage(message))
            static_cast<oCNpcMessage *>(message)->Delete();
    }
}

void ResetStuckWatch()
{
    StuckReference = HeroPosition();
    StuckReferenceTime = NowMs();
}

void StartDriving(const char *why)
{
    ClearMovement();
    ReleaseKeys();
    CurrentPhase = Phase::Driving;
    PhaseStartedAt = NowMs();
    DriveJumped = false;
    DriveJumpUntil = 0.0f;
    ResetStuckWatch();

    YagaLog::Write(std::format("[nav] driving ({}) {}/{} -> {} ({} m)", why, Index + 1, Path.size(),
                               WaypointName(Path[Index]),
                               Meters(DistanceXZ(HeroPosition(), Path[Index]->GetPositionWorld()))));
}

void IssueStep()
{
    ClearMovement();
    ReleaseKeys();
    CurrentPhase = Phase::Walking;

    // NPC movement cannot swim.
    if (HeroInWater() && Settings::NavigatorMode != 1)
    {
        StartDriving("in water");
        return;
    }

    oCMsgMovement *message = nullptr;
    if (Settings::NavigatorMode == 1)
        message = new oCMsgMovement(oCMsgMovement::EV_GOROUTE, TargetWaypoint->GetName());
    else
        message = new oCMsgMovement(oCMsgMovement::EV_GOTOPOS, Path[Index]->GetPositionWorld());

    player->GetEM()->OnMessage(message, player);
    ResetStuckWatch();

    YagaLog::Write(std::format("[nav] step {}/{} -> {} ({} m)", Index + 1, Path.size(), WaypointName(Path[Index]),
                               Meters(DistanceXZ(HeroPosition(), Path[Index]->GetPositionWorld()))));
}

std::string DescribeTarget()
{
    if (HasTargetSector)
        return std::format("sector {} {}", TargetSector.x, TargetSector.y);
    return WaypointName(TargetWaypoint);
}

void Stop(const StopReason reason)
{
    if (!Active)
        return;

    Active = false;
    ReleaseKeys();

    const bool worldStillValid = reason != StopReason::WorldChanged;
    if (worldStillValid && player)
    {
        ClearMovement();
        if (player->anictrl)
            player->anictrl->_Stand();
    }

    const float seconds = (NowMs() - StartedAt) / 1000.0f;
    YagaLog::Write(std::format("[nav] stop reason {} after {:.1f} s, {} recalculations, {} blocked legs",
                               static_cast<int>(reason), seconds, Recalculations, BlockedLegs.size()));

    switch (reason)
    {
    case StopReason::Arrived:
        Speech::ReadFormat("Arrived, {}.", DescribeTarget());
        break;
    case StopReason::Cancelled:
        Speech::Read("Navigation cancelled.");
        break;
    case StopReason::Attacked:
        Speech::Read("Navigation stopped, you are under attack.");
        break;
    case StopReason::NoRoute:
        Speech::ReadFormat("No way through to {}.", DescribeTarget());
        break;
    case StopReason::GaveUp:
        Speech::ReadFormat("Navigation stopped, cannot get through. {} meters left.",
                           Meters(RemainingLength(HeroPosition(), Path, Index)));
        break;
    case StopReason::WorldChanged:
    case StopReason::Dead:
        break;
    }

    Path.clear();
    Index = 0;
    TargetWaypoint = nullptr;
}

// Goals: every waypoint of the sector, nearest to its centre first; or one named waypoint.
std::vector<zCWaypoint *> CurrentGoals(zCWayNet *wayNet)
{
    if (HasTargetSector)
        return WaypointsInSector(wayNet, TargetSector);
    if (TargetWaypoint)
        return {TargetWaypoint};
    return {};
}

bool Begin(const std::vector<zCWaypoint *> &goals)
{
    zCWayNet *wayNet = GetWayNet();
    if (!player || !wayNet)
        return false;

    if (goals.empty())
    {
        Speech::ReadFormat("No waypoints in {}, unreachable area.", DescribeTarget());
        return false;
    }

    if (Active)
        ClearMovement();

    BlockedLegs.clear();
    BlockedStarts.clear();
    TriedDoors.clear();

    Plan plan;
    const bool found = PlanRoute(wayNet, HeroPosition(), goals, plan);
    LogPlan(found ? "plan" : "plan failed", plan);
    if (!found)
    {
        Active = false;
        Speech::ReadFormat("No route to {}.", DescribeTarget());
        return false;
    }

    Active = true;
    Path = plan.Waypoints;
    Index = 0;
    TargetWaypoint = plan.Target;
    NavWorld = ogame->GetGameWorld();
    NavWayNet = wayNet;
    RetriesOnWaypoint = 0;
    Recalculations = 0;
    WaypointsSinceAnnouncement = 0;
    LastHitpoints = player->GetAttribute(NPC_ATR_HITPOINTS);
    StartedAt = NowMs();

    Speech::ReadFormat("Walking to {}, {} meters, {} waypoints.", DescribeTarget(), Meters(plan.Length),
                       plan.Waypoints.size());
    IssueStep();
    return true;
}

bool StartToSector(const SECTOR &sector)
{
    zCWayNet *wayNet = GetWayNet();
    if (!player || !wayNet)
        return false;

    HasTargetSector = true;
    TargetSector = sector;
    TargetWaypoint = nullptr;
    return Begin(WaypointsInSector(wayNet, sector));
}

bool StartToWaypoint(const zSTRING &name)
{
    zCWayNet *wayNet = GetWayNet();
    if (!player || !wayNet)
        return false;

    zCWaypoint *wp = wayNet->GetWaypoint(name);
    if (!wp)
    {
        Speech::ReadFormat("Waypoint {} not found.", name.ToChar());
        return false;
    }

    HasTargetSector = false;
    TargetWaypoint = wp;
    return Begin({wp});
}

// Plan again from where the hero stands, keeping the same destination and leaving out blocked legs.
void Recalculate(const char *why)
{
    ++Recalculations;
    if (Recalculations > MaxRecalculations)
    {
        Stop(StopReason::GaveUp);
        return;
    }

    zCWayNet *wayNet = GetWayNet();
    Plan plan;
    const bool found = PlanRoute(wayNet, HeroPosition(), CurrentGoals(wayNet), plan);
    LogPlan(found ? "recalculated" : "recalculation failed", plan);
    if (!found)
    {
        Stop(StopReason::NoRoute);
        return;
    }

    Speech::ReadFormat("{}, recalculating. {} meters.", why, Meters(plan.Length));
    Path = plan.Waypoints;
    Index = 0;
    TargetWaypoint = plan.Target;
    RetriesOnWaypoint = 0;
    IssueStep();
}

// The hero could not walk the current leg: remember it so the next plan goes around it.
void BlockCurrentLeg()
{
    if (Index > 0)
    {
        BlockedLegs.insert(MakeLeg(Path[Index - 1], Path[Index]));
        YagaLog::Write(std::format("[nav] blocked leg {} - {}", WaypointName(Path[Index - 1]),
                                   WaypointName(Path[Index])));
    }
    else
    {
        BlockedStarts.insert(Path[0]);
        YagaLog::Write("[nav] blocked start " + WaypointName(Path[0]));
    }
}

// oCMobDoor also covers other two-state mobs (beds are doors to the engine), so look at the interaction scheme, which
// the engine takes from the model name ("DOOR" for DOOR_WOODEN).
bool IsDoor(oCMobDoor *mob)
{
    std::string scheme = mob->sceme.ToChar();
    std::transform(scheme.begin(), scheme.end(), scheme.begin(), [](unsigned char c) { return std::toupper(c); });
    return scheme.find("DOOR") != std::string::npos;
}

// A closed door near the hero, between it and the waypoint it is heading for, or right next to it.
oCMobDoor *FindClosedDoor()
{
    const zVEC3 hero = HeroPosition();
    const zVEC3 target = Path[Index]->GetPositionWorld();
    oCMobDoor *best = nullptr;
    float bestDistance = DoorSearchRadius;

    for (zCListSort<zCVob> *node = ogame->GetGameWorld()->voblist; node; node = node->next)
    {
        oCMobDoor *door = zDYNAMIC_CAST<oCMobDoor>(node->data);
        if (!door || TriedDoors.count(door) || door->GetState() != 0 || !IsDoor(door))
            continue;

        const zVEC3 position = door->GetPositionWorld();
        const float distance = hero.Distance(position);
        if (distance >= bestDistance)
            continue;

        // Only doors on the way: not behind the hero.
        if (DistanceToLegXZ(position, hero, target) > 250.0f && distance > 250.0f)
            continue;

        best = door;
        bestDistance = distance;
    }

    return best;
}

bool TryOpenDoor()
{
    oCMobDoor *door = FindClosedDoor();
    if (!door)
        return false;

    TriedDoors.insert(door);
    if (door->locked)
    {
        Speech::Read("Locked door.");
        YagaLog::Write("[nav] locked door " + std::string(door->GetObjectName().ToChar()));
        return false;
    }

    // Open it the way a player does: face the door and hold action + forward. An EV_USEMOB message would be the NPC
    // way, but the player's event manager never finishes it.
    ClearMovement();
    ReleaseKeys();
    zVEC3 look = door->GetPositionWorld();
    look[VY] = HeroPosition()[VY];
    player->SetHeadingYWorld(look);

    DoorBeingOpened = door;
    CurrentPhase = Phase::OpeningDoor;
    PhaseStartedAt = NowMs();
    YagaLog::Write(std::format("[nav] opening door {} at {} m", door->GetObjectName().ToChar(),
                               Meters(HeroPosition().Distance(door->GetPositionWorld()))));
    return true;
}

// The current step failed (the engine gave up, or the hero stopped moving).
void OnStepFailed(const char *what)
{
    YagaLog::Write(std::format("[nav] step failed ({}), {} m from {}", what,
                               Meters(DistanceXZ(HeroPosition(), Path[Index]->GetPositionWorld())),
                               WaypointName(Path[Index])));

    if (TryOpenDoor())
        return;

    // The engine gave up: walk the rest of the leg ourselves, jumping if needed.
    if (CurrentPhase == Phase::Walking)
    {
        StartDriving("engine gave up");
        return;
    }

    if (++RetriesOnWaypoint <= MaxRetriesPerWaypoint && !HeroInWater())
    {
        IssueStep();
        return;
    }

    ReleaseKeys();
    BlockCurrentLeg();
    Recalculate("Blocked");
}

void UpdateDriving(const zVEC3 &hero)
{
    zVEC3 target = Path[Index]->GetPositionWorld();
    target[VY] = hero[VY];

    // Never push a hero on dry land into deep water.
    if (!HeroInWater())
    {
        zVEC3 direction = target - hero;
        direction[VY] = 0.0f;
        if (direction.LengthApprox() > 1.0f)
        {
            const zVEC3 ahead = hero + direction.Normalize() * 150.0f;
            if (WaterDepthAt(ahead) > DeepWater)
            {
                ReleaseKeys();
                YagaLog::Write("[nav] driving: deep water ahead");
                BlockCurrentLeg();
                Recalculate("Water ahead");
                return;
            }
        }
    }

    player->SetHeadingYWorld(target);

    Inject(GAME_UP, true);
    Inject(GAME_SMOVE, NowMs() < DriveJumpUntil);

    if (DistanceXZ(hero, StuckReference) > StuckMinMove)
    {
        ResetStuckWatch();
        DriveJumped = false;
        return;
    }

    const float idle = NowMs() - StuckReferenceTime;
    if (!DriveJumped && idle > DriveJumpAfterMs)
    {
        DriveJumped = true;
        DriveJumpUntil = NowMs() + DriveJumpHoldMs;
        YagaLog::Write("[nav] driving: no progress, jumping");
        return;
    }

    if (idle > DriveGiveUpMs)
        OnStepFailed("driving made no progress");
}

void AdvanceWaypoint()
{
    ++Index;
    RetriesOnWaypoint = 0;

    if (Index >= Path.size())
    {
        Stop(StopReason::Arrived);
        return;
    }

    ++WaypointsSinceAnnouncement;
    if (Settings::NavigatorProgressEvery > 0 && WaypointsSinceAnnouncement >= Settings::NavigatorProgressEvery)
    {
        WaypointsSinceAnnouncement = 0;
        Speech::ReadFormat("{} meters.", Meters(RemainingLength(HeroPosition(), Path, Index)));
    }

    if (Settings::NavigatorMode != 1)
        IssueStep();
}

bool PlayerWantsControl()
{
    static const unsigned short movementKeys[] = {GAME_UP,    GAME_DOWN,       GAME_LEFT,
                                                  GAME_RIGHT, GAME_STRAFELEFT, GAME_STRAFERIGHT};
    for (const unsigned short key : movementKeys)
        if (RealState(key) > 0.0f)
            return true;
    return false;
}

void SpeakStatus()
{
    if (!Active)
    {
        Speech::Read("Navigator idle.");
        return;
    }

    Speech::ReadFormat("Walking to {}, {} meters left, waypoint {} of {}.", DescribeTarget(),
                       Meters(RemainingLength(HeroPosition(), Path, Index)), Index + 1, Path.size());
}

void UpdateEngineRoute(const zVEC3 &hero, const bool engineBusy)
{
    while (Index + 1 < Path.size() && DistanceXZ(hero, Path[Index]->GetPositionWorld()) < ArriveRadius * 2.0f)
        AdvanceWaypoint();

    if (!engineBusy)
    {
        if (DistanceXZ(hero, Path.back()->GetPositionWorld()) < EngineDoneRadius)
            Stop(StopReason::Arrived);
        else
            Recalculate("Route interrupted");
    }
}

void Update()
{
    if (!Active)
        return;

    if (!player || !ogame || ogame->GetGameWorld() != NavWorld || GetWayNet() != NavWayNet)
    {
        Stop(StopReason::WorldChanged);
        return;
    }

    if (player->IsDead())
    {
        Stop(StopReason::Dead);
        return;
    }

    if (PlayerWantsControl())
    {
        Stop(StopReason::Cancelled);
        return;
    }

    const int hitpoints = player->GetAttribute(NPC_ATR_HITPOINTS);
    if (hitpoints < LastHitpoints)
    {
        Stop(StopReason::Attacked);
        return;
    }
    LastHitpoints = hitpoints;

    const zVEC3 hero = HeroPosition();
    const bool engineBusy = HasPendingMessage();

    if (CurrentPhase == Phase::OpeningDoor)
    {
        const float elapsed = NowMs() - PhaseStartedAt;
        const bool pressing = elapsed < DoorPressMs;
        Inject(GAME_ACTION, pressing);
        Inject(GAME_UP, pressing);

        const bool open = DoorBeingOpened && DoorBeingOpened->GetState() != 0;
        if (open || elapsed > DoorTimeoutMs)
        {
            ReleaseKeys();
            YagaLog::Write(std::format("[nav] door {} after {:.1f} s", open ? "open" : "still closed",
                                       elapsed / 1000.0f));
            if (open)
                Speech::Read("Door opened.", 0);
            DoorBeingOpened = nullptr;
            IssueStep();
        }
        return;
    }

    if (Settings::NavigatorMode == 1)
    {
        UpdateEngineRoute(hero, engineBusy);
        return;
    }

    const float toWaypoint = DistanceXZ(hero, Path[Index]->GetPositionWorld());
    if (CurrentPhase == Phase::Driving)
    {
        if (toWaypoint < ArriveRadius)
        {
            ReleaseKeys();
            AdvanceWaypoint();
        }
        else
            UpdateDriving(hero);
        return;
    }

    if (toWaypoint < ArriveRadius || (!engineBusy && toWaypoint < EngineDoneRadius))
    {
        AdvanceWaypoint();
        return;
    }

    if (!engineBusy)
    {
        OnStepFailed("engine gave up");
        return;
    }

    // Pushed off the route (or fell): plan again from here.
    if (Index > 0 &&
        DistanceToLegXZ(hero, Path[Index - 1]->GetPositionWorld(), Path[Index]->GetPositionWorld()) > OffRouteDistance)
    {
        Recalculate("Off route");
        return;
    }

    // Stuck: no progress for a while.
    if (DistanceXZ(hero, StuckReference) > StuckMinMove)
    {
        ResetStuckWatch();
        return;
    }

    if (NowMs() - StuckReferenceTime > StuckWindowMs)
        OnStepFailed("no progress");
}

// The game is paused (menu): release held keys; Update holds them again once the game runs.
void OnPause()
{
    ReleaseKeys();
}

void Reset()
{
    Active = false;
    ReleaseKeys();
    DoorBeingOpened = nullptr;
    CurrentPhase = Phase::Walking;
    Path.clear();
    Index = 0;
    TargetWaypoint = nullptr;
    NavWorld = nullptr;
    NavWayNet = nullptr;
    BlockedLegs.clear();
    BlockedStarts.clear();
    TriedDoors.clear();
}
} // namespace Navigator

void NavigatorLoop()
{
    if (!player || !zinput)
        return;

    if (zinput->KeyToggled(KEY_J))
        Navigator::SpeakStatus();

    Navigator::Update();
}
} // namespace GOTHIC_NAMESPACE
