#include <algorithm>
#include <cctype>
#include <chrono>
#include <map>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

namespace GOTHIC_NAMESPACE
{
// Route planning and auto-walk.
//
// Routes are planned on the navigation mesh (NavMesh.hpp): every surface the hero can walk on, made from the world's
// collision geometry. Its corners become temporary waypoints, never put into the way net. Until the mesh is ready,
// or where the hero stands off it, the graph is the game's own way net: the waypoints and ways NPCs walk on. That
// search is done here (Dijkstra over the way net) instead of with zCWayNet::FindRoute, because a recalculation has to
// leave out the legs the hero already failed to walk, and the engine search cannot be told to do that.
//
// Walking hands the hero to the engine's NPC movement (oCMsgMovement EV_GOTOPOS), whose "robust trace" turns around
// small obstacles. Where that movement cannot go (swimming, or a ledge it gives up on), the navigator drives the hero
// itself with the engine's animation controller: it faces the waypoint and runs (_Forward), and jumps or climbs
// (PC_JumpForward) when the hero stops moving. Doors and ladders are used through the mob interaction functions
// (StartInteraction, Interact, StopInteraction). No key is ever pressed or held for the player. It also notices when
// the hero is stuck or pushed off the route, and then recalculates around the blocked leg.
namespace Navigator
{
constexpr float ArriveRadius = 150.0f;       // a waypoint counts as reached within this XZ distance (cm)
constexpr float EngineDoneRadius = 400.0f;   // ... or within this one when the engine reports the move as finished
constexpr float ArriveHeight = 300.0f;       // ... and no farther above or below (ladders and ledges are vertical)
constexpr float StartSearchRadius = 5000.0f; // waypoints considered as the first step of a route
constexpr float StuckWindowMs = 2500.0f;     // no progress for this long counts as stuck
constexpr float StuckMinMove = 40.0f;        // ... where progress means moving at least this far
constexpr float OffRouteDistance = 1500.0f;  // farther than this from the current leg triggers a recalculation
constexpr float DoorSearchRadius = 600.0f;   // closed doors this close to the hero are opened when a step fails
constexpr float DoorTimeoutMs = 3000.0f;     // a door that is not open by then is given up on
constexpr float DriveJumpAfterMs = 1200.0f;  // driving without progress this long: try a jump / climb
constexpr float DriveJumpHoldMs = 300.0f;
constexpr float DriveGiveUpMs = 4000.0f;     // driving without progress this long: the leg is blocked
// Water deeper than this makes the hero swim. NPC movement cannot swim, so the navigator swims such legs itself, and
// the search makes them cost more than walking, so a dry way round wins when there is one. Water that kills a
// swimming hero (the lake below the intro cliff) is kept out of routes by "avoid" entries in the waynet patch.
constexpr float DeepWater = 100.0f;
constexpr float SwimCostFactor = 4.0f;
constexpr float StepBaseMs = 15000.0f;       // a single leg may take this long plus 10 ms per cm (1 m/s) before it
constexpr float StepMsPerCm = 10.0f;         // counts as failed, so circling round an obstacle cannot last forever
constexpr float CloseEnough = 600.0f;        // giving up this close to the goal waypoint still counts as arrived
constexpr float LadderReachDistance = 250.0f; // the hero gets on a ladder from this close
constexpr float LadderMountMs = 4000.0f;
constexpr float LadderClimbMs = 20000.0f;
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
    OpeningDoor,
    Ladder       // gets on a ladder (action + forward), then climbs it up or down
};

// Legs and first steps a route must leave out. A navigation keeps its own set; planning for a test command uses a
// fresh one, so results do not leak between plans.
struct Blocked
{
    std::set<std::pair<zCWaypoint *, zCWaypoint *>> Legs; // legs the hero failed to walk
    std::set<zCWaypoint *> Starts;                         // waypoints the hero failed to reach from where it stood
    std::set<int> Nodes;                                   // mesh polygons the hero could not get through
    std::set<int> LinkTags;                                // ladders the hero could not use

    size_t Count() const
    {
        return Legs.size() + Starts.size() + Nodes.size() + LinkTags.size();
    }
};

enum class TargetKind
{
    None,
    Sector,
    Waypoint,
    Position
};

// How the last navigation ended, for the test tour.
struct Outcome
{
    StopReason Reason = StopReason::Cancelled;
    float Seconds = 0.0f;
    int Recalculations = 0;
    int BlockedLegs = 0;
    int MetersLeft = 0;
    bool ArrivedInSector = false; // gave up, but already inside the target sector (or near the target point)
};

struct Plan
{
    std::vector<zCWaypoint *> Waypoints;
    zCWaypoint *Start = nullptr;
    zCWaypoint *Target = nullptr;
    float Length = 0.0f; // from the hero through every waypoint, in cm
    double Milliseconds = 0.0;
    int Reachable = 0; // waypoints the search reached (mesh: polygons of the hero's island)
    bool Mesh = false; // planned on the navigation mesh: the waypoints are temporary
    std::vector<NavCore::LegKind> Legs; // mesh: how the leg into each waypoint is walked
    std::vector<int> Tags;              // mesh: the ladder of a ladder leg
    int ClosedGates = 0;                // mesh: polygons left out because a gate is down
};

using Leg = std::pair<zCWaypoint *, zCWaypoint *>;

inline bool Active = false;
inline Phase CurrentPhase = Phase::Walking;
inline std::vector<zCWaypoint *> Path;
inline size_t Index = 0;
inline zCWaypoint *TargetWaypoint = nullptr; // the goal the current plan leads to
inline TargetKind Kind = TargetKind::None;
inline SECTOR TargetSector{};
inline zCWaypoint *RequestedWaypoint = nullptr;
inline zVEC3 TargetPosition;
inline std::wstring TargetPlaceName;            // a named place in the target sector, if any
inline zCWaypoint *TargetPlaceWaypoint = nullptr; // ... and its waypoint, tried before the rest of the sector
inline bool UseEngineRoute = false; // NavigatorMode 1, only while every leg is a way net way
inline Outcome LastOutcome;
inline int Outcomes = 0; // counts finished navigations
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
inline float StepStartedAt = 0.0f;
inline float StepLimitMs = 0.0f;
inline Blocked NavBlocked;                      // what the current navigation has learned to avoid
inline std::set<oCMobDoor *> TriedDoors;        // doors already opened (or found locked) during this navigation
inline std::set<zCVob *> ReportedGates;         // closed gates already announced during this navigation
inline float DriveJumpStart = 0.0f;
inline float DriveJumpUntil = 0.0f;
inline int DriveJumps = 0;          // jumps tried since the hero last made progress
inline bool DriveClimbing = false;  // the current jump is a standing one, which climbs a ledge
inline float DriveBest = 0.0f;      // the hero's best distance to the waypoint (XZ plus height) while driving
inline oCMobDoor *DoorBeingOpened = nullptr;
inline oCMobLadder *LadderInUse = nullptr;
inline bool LadderMounted = false;
inline bool LadderUp = true;
inline bool LegWet = false;                          // the current leg goes through deep water
inline std::map<std::pair<zCWaypoint *, zCWaypoint *>, bool> WetLegs; // cache: does a leg cross deep water
inline bool MeshRoute = false;                    // Path comes from the navigation mesh
inline std::vector<NavCore::LegKind> LegKinds;    // ... how the leg into each waypoint is walked
inline std::vector<int> LegTags;                  // ... the ladder of a ladder leg
inline std::vector<zCWaypoint *> MeshWaypoints;   // ... the temporary waypoints, released with the path

inline bool Running = false; // the navigator started the hero's run with _Forward and has to stop it

// ---------------------------------------------------------------- helpers

// The hero runs forward through the animation controller, the way the player's own AI does it for the forward key.
void RunForward()
{
    if (player && player->anictrl)
    {
        player->anictrl->_Forward();
        Running = true;
    }
}

// Stop a run the navigator started.
void Halt()
{
    if (Running && player && player->anictrl)
        player->anictrl->_Stand();
    Running = false;
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
    zCWayNet *wayNet = world ? world->wayNet : nullptr;
    if (wayNet)
        WaynetPatch::EnsureBuilt();
    return wayNet;
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

bool CrossesDeepWater(const zVEC3 &from, const zVEC3 &to);

// Whether a leg goes through deep water, traced once per leg and remembered.
bool IsWetLeg(zCWaypoint *a, zCWaypoint *b)
{
    const Leg leg = MakeLeg(a, b);
    const auto found = WetLegs.find(leg);
    if (found != WetLegs.end())
        return found->second;
    const bool wet = CrossesDeepWater(a->GetPositionWorld(), b->GetPositionWorld());
    WetLegs[leg] = wet;
    return wet;
}

bool IsLegBlocked(const Blocked &blocked, zCWaypoint *a, zCWaypoint *b)
{
    return blocked.Legs.count(MakeLeg(a, b)) != 0 || WaynetPatch::IsAvoided(a, b);
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
// the ground found by one that does not. Collidable vobs count as ground, so a wooden bridge or boardwalk over water
// is dry.
float WaterDepthAt(const zVEC3 &position)
{
    oCWorld *world = ogame->GetGameWorld();
    const zVEC3 origin = position + zVEC3(0.0f, 250.0f, 0.0f);
    const zVEC3 ray(0.0f, -1500.0f, 0.0f);
    const int solid = zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER |
                      zTRACERAY_POLY_IGNORE_TRANSP;

    if (!world->TraceRayNearestHit(origin, ray, player, solid | zTRACERAY_POLY_TEST_WATER))
        return 0.0f;

    if (world->traceRayReport.foundVob)
        return 0.0f; // a bridge, a raft, a boardwalk
    zCPolygon *poly = world->traceRayReport.foundPoly;
    if (!poly || !poly->material || poly->material->matGroup != zMAT_GROUP_WATER)
        return 0.0f;

    const float surface = world->traceRayReport.foundIntersection[VY];
    if (!world->TraceRayNearestHit(origin, ray, player, solid))
        return 1500.0f;

    return (std::max)(0.0f, surface - world->traceRayReport.foundIntersection[VY]);
}

// Samples every 3 m along the line.
bool CrossesDeepWater(const zVEC3 &from, const zVEC3 &to)
{
    const int samples = (std::max)(4, static_cast<int>(DistanceXZ(from, to) / 300.0f));
    for (int i = 1; i <= samples; ++i)
        if (WaterDepthAt(from + (to - from) * (static_cast<float>(i) / samples)) > DeepWater)
            return true;
    return false;
}

// Way net waypoints plus the patch nodes.
template <typename Fn> void ForEachWaypoint(zCWayNet *wayNet, Fn fn)
{
    for (auto node = wayNet->wplist.next; node; node = node->next)
        if (node->data)
            fn(node->data);
    if (WaynetPatch::BuiltFor == wayNet)
        for (zCWaypoint *wp : WaynetPatch::Nodes)
            fn(wp);
}

zCWaypoint *OtherEnd(zCWay *way, zCWaypoint *from)
{
    if (way->left == from)
        return way->right;
    if (way->right == from)
        return way->left;
    return nullptr;
}

// Way net ways plus patch links.
template <typename Fn> void ForEachNeighbour(zCWaypoint *wp, Fn fn)
{
    for (auto node = wp->wayList.next; node; node = node->next)
        if (zCWaypoint *next = node->data ? OtherEnd(node->data, wp) : nullptr)
            fn(next);
    WaynetPatch::ForEachLink(wp, fn);
}

// A waypoint by name: a patch node or a way net waypoint (names are upper case).
zCWaypoint *FindWaypoint(const std::string &name)
{
    return WaynetPatch::Find(GetWayNet(), name);
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

// Close enough to a waypoint, in the XZ plane and in height.
bool Within(const zVEC3 &hero, zCWaypoint *wp, const float radius)
{
    const zVEC3 position = wp->GetPositionWorld();
    return DistanceXZ(hero, position) < radius && std::abs(hero[VY] - position[VY]) < ArriveHeight;
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
// Candidates for the first waypoint: the ones the hero can see, nearest first, then the nearest one at all. More than
// one, because the nearest can belong to a patch of waypoints cut off from the destination (a tower interior seen
// through its door).
std::vector<zCWaypoint *> StartCandidates(zCWayNet *wayNet, const zVEC3 &from, const Blocked &blocked)
{
    std::vector<std::pair<float, zCWaypoint *>> nearby;
    ForEachWaypoint(wayNet, [&](zCWaypoint *wp) {
        if (blocked.Starts.count(wp))
            return;
        const float distance = from.Distance(wp->GetPositionWorld());
        if (distance <= StartSearchRadius)
            nearby.emplace_back(distance, wp);
    });

    std::sort(nearby.begin(), nearby.end(),
              [](const auto &left, const auto &right) { return left.first < right.first; });

    std::vector<zCWaypoint *> result;
    const size_t checks = (std::min)(nearby.size(), static_cast<size_t>(MaxStartCandidates));
    for (size_t i = 0; i < checks; ++i)
        if (HasClearLine(from, nearby[i].second->GetPositionWorld()))
            result.push_back(nearby[i].second);

    if (!nearby.empty() && std::find(result.begin(), result.end(), nearby.front().second) == result.end())
        result.push_back(nearby.front().second);
    if (result.empty())
        if (zCWaypoint *nearest = wayNet->GetNearestWaypoint(from))
            result.push_back(nearest);
    return result;
}

zCWaypoint *ChooseStartWaypoint(zCWayNet *wayNet, const zVEC3 &from, const Blocked &blocked)
{
    std::vector<std::pair<float, zCWaypoint *>> nearby;
    ForEachWaypoint(wayNet, [&](zCWaypoint *wp) {
        if (blocked.Starts.count(wp))
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

// Goals for a point: the waypoints nearest to it (XZ), so a route can end at the next best one when the nearest is
// out of reach.
std::vector<zCWaypoint *> WaypointsNear(zCWayNet *wayNet, const zVEC3 &position, const size_t count = 12)
{
    std::vector<std::pair<float, zCWaypoint *>> all;
    ForEachWaypoint(wayNet,
                    [&](zCWaypoint *wp) { all.emplace_back(DistanceXZ(wp->GetPositionWorld(), position), wp); });
    std::sort(all.begin(), all.end(), [](const auto &left, const auto &right) { return left.first < right.first; });

    std::vector<zCWaypoint *> result;
    for (size_t i = 0; i < all.size() && i < count; ++i)
        result.push_back(all[i].second);
    return result;
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
void ShortestPaths(zCWaypoint *start, const Blocked &blocked, std::unordered_map<zCWaypoint *, float> &distance,
                   std::unordered_map<zCWaypoint *, zCWaypoint *> &parent)
{
    using Item = std::pair<float, zCWaypoint *>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> open;

    distance[start] = 0.0f;
    open.push({0.0f, start});
    while (!open.empty())
    {
        const float cost = open.top().first;
        zCWaypoint *wp = open.top().second;
        open.pop();
        if (cost > distance[wp])
            continue;

        ForEachNeighbour(wp, [&](zCWaypoint *next) {
            if (IsLegBlocked(blocked, wp, next))
                return;

            const float length = wp->GetPositionWorld().Distance(next->GetPositionWorld());
            const float nextCost = cost + (IsWetLeg(wp, next) ? length * SwimCostFactor : length);
            const auto found = distance.find(next);
            if (found == distance.end() || nextCost < found->second)
            {
                distance[next] = nextCost;
                parent[next] = wp;
                open.push({nextCost, next});
            }
        });
    }
}

// Skip leading waypoints that would make the hero walk back: while the following waypoint is in sight and no farther
// from the hero than from the waypoint before it, head straight for it.
void TrimLeadingWaypoints(std::vector<zCWaypoint *> &waypoints, const zVEC3 &from, const Blocked &blocked)
{
    while (waypoints.size() >= 2 && !blocked.Starts.count(waypoints[1]))
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
bool PlanRouteOnce(zCWayNet *wayNet, const zVEC3 &from, const std::vector<zCWaypoint *> &goals, Plan &plan,
                   const Blocked &blocked)
{
    const auto started = std::chrono::high_resolution_clock::now();
    const auto elapsed = [&]() {
        return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - started).count();
    };

    plan = Plan();
    std::unordered_map<zCWaypoint *, float> distance;
    std::unordered_map<zCWaypoint *, zCWaypoint *> parent;
    const std::vector<zCWaypoint *> starts = StartCandidates(wayNet, from, blocked);
    for (size_t i = 0; i < starts.size() && i < 6 && !plan.Target; ++i)
    {
        distance.clear();
        parent.clear();
        plan.Start = starts[i];
        ShortestPaths(plan.Start, blocked, distance, parent);
        plan.Reachable = static_cast<int>(distance.size());

        for (zCWaypoint *goal : goals)
        {
            if (distance.count(goal))
            {
                plan.Target = goal;
                break;
            }
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

    TrimLeadingWaypoints(plan.Waypoints, from, blocked);
    plan.Length = RemainingLength(from, plan.Waypoints, 0);
    plan.Milliseconds = elapsed();
    return true;
}

// Swimming legs are part of the search (at a higher cost), so one search is enough.
bool PlanRoute(zCWayNet *wayNet, const zVEC3 &from, const std::vector<zCWaypoint *> &goals, Plan &plan,
               Blocked &blocked)
{
    return PlanRouteOnce(wayNet, from, goals, plan, blocked);
}

// ---------------------------------------------------------------- planning on the navigation mesh

// A mesh corner as a waypoint the walking code can use. It is never put into the way net, so NPC routes and
// savegames do not see it. Named after its position in metres, for the log.
zCWaypoint *MakeMeshWaypoint(const float *pos)
{
    zCWaypoint *wp = new zCWaypoint();
    zSTRING name(std::format("MESH_{}_{}", static_cast<int>(pos[0] / 100.0f), static_cast<int>(pos[2] / 100.0f))
                     .c_str());
    wp->SetName(name);
    zVEC3 position(pos[0], pos[1] + 50.0f, pos[2]); // like the way net: half a metre above the ground
    wp->Init(position);
    return wp;
}

void ReleaseWaypoints(std::vector<zCWaypoint *> &waypoints)
{
    for (zCWaypoint *wp : waypoints)
        wp->Release(); // never Free(): that deletes ways and the waypoint vob
    waypoints.clear();
}

// Goals on the mesh for the current target, best first: a point and its polygon. For a sector, its named place, then
// polygons of the sector the hero can reach, nearest to the sector's centre first.
std::vector<std::pair<zVEC3, int>> MeshGoals(const int island)
{
    std::vector<std::pair<zVEC3, int>> goals;
    const auto add = [&](const zVEC3 &position) {
        zVEC3 snapped;
        const int node = NavMesh::NodeAt(position, &snapped);
        if (node >= 0)
            goals.emplace_back(snapped, node);
    };

    switch (Kind)
    {
    case TargetKind::Waypoint:
        if (RequestedWaypoint)
            add(RequestedWaypoint->GetPositionWorld());
        break;
    case TargetKind::Position:
        add(TargetPosition);
        break;
    case TargetKind::Sector:
    {
        if (TargetPlaceWaypoint)
            add(TargetPlaceWaypoint->GetPositionWorld());
        const zVEC3 center = SectorCenter(TargetSector);
        int added = 0;
        for (const int node : NavMesh::SectorNodes(TargetSector))
        {
            if (NavMesh::IslandOf(node) != island)
                continue;
            zVEC3 point = center;
            point[VY] = NavMesh::Current->graph.Center(node)[1];
            goals.emplace_back(NavMesh::ClosestPointOn(node, point), node);
            if (++added >= 4)
                break;
        }
        break;
    }
    default:
        break;
    }
    return goals;
}

// A route on the navigation mesh from the hero to the current target. False when the mesh is not ready, the hero is
// not on it, or the target cannot be reached on it.
bool PlanMeshRoute(const zVEC3 &from, Plan &plan, const Blocked &blocked)
{
    const auto started = std::chrono::high_resolution_clock::now();
    plan = Plan();
    plan.Mesh = true;
    if (!NavMesh::Ready())
        return false;

    zVEC3 start;
    const int startNode = NavMesh::NodeAt(from, &start);
    if (startNode < 0)
    {
        YagaLog::Write("[nav] the hero is off the navigation mesh");
        return false;
    }
    const int island = NavMesh::IslandOf(startNode);
    plan.Reachable = NavMesh::Current->graph.IslandSize(island);

    std::vector<NavCore::Corner> corners;
    bool found = false;
    for (const auto &[position, node] : MeshGoals(island))
        if (NavMesh::FindRoute(startNode, start, node, position, blocked.Nodes, blocked.LinkTags, corners,
                               plan.ClosedGates))
        {
            found = true;
            break;
        }
    plan.Milliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - started).count();
    if (!found)
        return false;

    for (const NavCore::Corner &corner : corners)
    {
        plan.Waypoints.push_back(MakeMeshWaypoint(corner.pos));
        plan.Legs.push_back(corner.leg);
        plan.Tags.push_back(corner.tag);
    }
    plan.Start = plan.Waypoints.front();
    plan.Target = plan.Waypoints.back();
    plan.Length = RemainingLength(from, plan.Waypoints, 0);
    return true;
}

void LogPlan(const char *what, const Plan &plan, const Blocked &blocked)
{
    std::string names;
    for (size_t i = 0; i < plan.Waypoints.size(); ++i)
    {
        if (i)
            names += " > ";
        if (plan.Mesh && i < plan.Legs.size())
        {
            if (plan.Legs[i] == NavCore::LegKind::Swim)
                names += "~";
            else if (plan.Legs[i] == NavCore::LegKind::Link)
                names += "#";
        }
        names += WaypointName(plan.Waypoints[i]);
    }

    YagaLog::Write(std::format("[nav] {}: {}, start {}, goal {}, {} waypoints, {} m, {} reachable, {} blocked, "
                               "{} closed gate polygons, {:.3f} ms",
                               what, plan.Mesh ? "mesh" : "way net", WaypointName(plan.Start),
                               WaypointName(plan.Target), plan.Waypoints.size(), Meters(plan.Length), plan.Reachable,
                               blocked.Count(), plan.ClosedGates, plan.Milliseconds));
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

void StartStepTimer()
{
    StepStartedAt = NowMs();
    StepLimitMs = StepBaseMs + StepMsPerCm * HeroPosition().Distance(Path[Index]->GetPositionWorld());
}

void StartDriving(const char *why)
{
    ClearMovement();
    Halt();
    StartStepTimer();
    CurrentPhase = Phase::Driving;
    PhaseStartedAt = NowMs();
    DriveJumps = 0;
    DriveJumpStart = 0.0f;
    DriveJumpUntil = 0.0f;
    DriveBest = 1e9f;
    ResetStuckWatch();

    YagaLog::Write(std::format("[nav] driving ({}) {}/{} -> {} ({} m)", why, Index + 1, Path.size(),
                               WaypointName(Path[Index]),
                               Meters(DistanceXZ(HeroPosition(), Path[Index]->GetPositionWorld()))));
}

// The ladder of a way net way. The engine marks such ways when it loads the way net, but the ladder it links is not
// always the one at the way (in the mountain fortress three ladders share a name), so the ladder actually used is
// the one nearest to the start of the leg.
oCMobLadder *LadderOnLeg(zCWaypoint *a, zCWaypoint *b)
{
    if (WaynetPatch::IsOurs(a) || WaynetPatch::IsOurs(b))
        return nullptr;
    zCWay *way = a->HasWay(b);
    oCMobLadder *marked = way ? static_cast<oCWay *>(way)->ladder : nullptr;
    const zVEC3 start = a->GetPositionWorld();
    const zVEC3 end = b->GetPositionWorld();

    // Not marked, but a climb straight up or down (a tower's floors): a ladder standing right there is the way.
    const bool vertical = DistanceXZ(start, end) < 300.0f && std::abs(end[VY] - start[VY]) > 250.0f;
    if (!marked && !vertical)
        return nullptr;

    oCMobLadder *best = marked;
    float bestDistance = marked ? DistanceXZ(start, marked->GetPositionWorld()) + 1.0f : 400.0f;
    for (zCListSort<zCVob> *node = ogame->GetGameWorld()->voblist; node; node = node->next)
    {
        oCMobLadder *ladder = zDYNAMIC_CAST<oCMobLadder>(node->data);
        if (!ladder)
            continue;
        const zVEC3 position = ladder->GetPositionWorld();
        const float distance = DistanceXZ(start, position) + std::abs(start[VY] - position[VY]) * 0.5f;
        if (distance < bestDistance)
        {
            best = ladder;
            bestDistance = distance;
        }
    }
    return best;
}

oCMobDoor *DoorOnLeg(zCWaypoint *a, zCWaypoint *b)
{
    if (WaynetPatch::IsOurs(a) || WaynetPatch::IsOurs(b))
        return nullptr;
    zCWay *way = a->HasWay(b);
    return way ? static_cast<oCWay *>(way)->door : nullptr;
}

// Use the ladder the way the engine does for any character: put the hero on its spot (SetIdealPosition), turn it
// (SetHeading) and start the interaction; then climb with Interact, up or down, until the hero is off at the other
// end.
void StartLadder(oCMobLadder *ladder)
{
    ClearMovement();
    Halt();
    LadderInUse = ladder;
    LadderMounted = false;
    CurrentPhase = Phase::Ladder;
    PhaseStartedAt = NowMs();
    ResetStuckWatch();
    YagaLog::Write(std::format("[nav] ladder {} {} at {} m", ladder->GetObjectName().ToChar(), LadderUp ? "up" : "down",
                               Meters(HeroPosition().Distance(ladder->GetPositionWorld()))));
}

void IssueStep()
{
    ClearMovement();
    Halt();
    CurrentPhase = Phase::Walking;
    StartStepTimer();

    if (MeshRoute)
        LegWet = Index < LegKinds.size() &&
                 (LegKinds[Index] == NavCore::LegKind::Swim ||
                  (LegKinds[Index] == NavCore::LegKind::Link && NavMesh::IsWetWay(LegTags[Index])));
    else
        LegWet = Index > 0 ? IsWetLeg(Path[Index - 1], Path[Index])
                           : CrossesDeepWater(HeroPosition(), Path[0]->GetPositionWorld());

    // A ladder: the mesh says which one and which way; on the way net it is found at the leg.
    if (MeshRoute && Index < LegKinds.size() && LegKinds[Index] == NavCore::LegKind::Link)
        if (oCMobLadder *ladder = NavMesh::LadderOfTag(LegTags[Index]))
        {
            LadderUp = LegTags[Index] % 2 == 0;
            StartLadder(ladder);
            return;
        }
    if (!MeshRoute && !UseEngineRoute && Index > 0)
        if (oCMobLadder *ladder = LadderOnLeg(Path[Index - 1], Path[Index]))
        {
            LadderUp = Path[Index]->GetPositionWorld()[VY] >= Path[Index - 1]->GetPositionWorld()[VY];
            StartLadder(ladder);
            return;
        }

    // A leg through deep water: NPC movement cannot swim.
    if (LegWet && !UseEngineRoute)
    {
        StartDriving("swim");
        return;
    }

    // NPC movement cannot swim.
    if (HeroInWater() && !UseEngineRoute)
    {
        StartDriving("in water");
        return;
    }

    // A patch leg marked "jump": NPC movement would stop at the ledge.
    if (!MeshRoute && !UseEngineRoute && Index > 0 && WaynetPatch::IsJumpLeg(Path[Index - 1], Path[Index]))
    {
        StartDriving("jump leg");
        return;
    }

    oCMsgMovement *message = nullptr;
    if (UseEngineRoute)
        message = new oCMsgMovement(oCMsgMovement::EV_GOROUTE, TargetWaypoint->GetName());
    else
        message = new oCMsgMovement(oCMsgMovement::EV_GOTOPOS, Path[Index]->GetPositionWorld());

    player->GetEM()->OnMessage(message, player);
    ResetStuckWatch();

    YagaLog::Write(std::format("[nav] step {}/{} -> {} ({} m)", Index + 1, Path.size(), WaypointName(Path[Index]),
                               Meters(DistanceXZ(HeroPosition(), Path[Index]->GetPositionWorld()))));
}

std::wstring DescribeTarget()
{
    switch (Kind)
    {
    case TargetKind::Sector:
        if (!TargetPlaceName.empty())
            return Loc::Format(L"{}, sector {} {}", L"{}, sektor {} {}", TargetPlaceName, TargetSector.x,
                               TargetSector.y);
        return Loc::Format(L"sector {} {}", L"sektor {} {}", TargetSector.x, TargetSector.y);
    case TargetKind::Waypoint:
        return ToWString(WaypointName(RequestedWaypoint));
    case TargetKind::Position:
        return Loc::Format(L"point {} {}", L"punkt {} {}", Meters(TargetPosition[VX]), Meters(TargetPosition[VZ]));
    default:
        return L"-";
    }
}

// The hero did not reach the goal waypoint, but stands where the player wanted to go anyway: inside the target
// sector, or close to the target point.
bool HeroAtDestination()
{
    const zVEC3 hero = HeroPosition();
    if (TargetWaypoint)
    {
        const zVEC3 goal = TargetWaypoint->GetPositionWorld();
        if (DistanceXZ(hero, goal) < CloseEnough && std::abs(hero[VY] - goal[VY]) < 300.0f)
            return true;
    }
    if (Kind == TargetKind::Sector)
        return SameSector(SectorOf(hero), TargetSector);
    if (Kind == TargetKind::Position)
        return DistanceXZ(hero, TargetPosition) < 1500.0f;
    return false;
}

void Stop(StopReason reason)
{
    if (!Active)
        return;

    Active = false;
    Halt();

    const bool worldStillValid = reason != StopReason::WorldChanged;
    bool arrivedInSector = false;
    if (worldStillValid && player && (reason == StopReason::GaveUp || reason == StopReason::NoRoute) &&
        HeroAtDestination())
    {
        YagaLog::Write(std::format("[nav] could not reach {}, but the hero is at the destination",
                                   WaypointName(TargetWaypoint)));
        reason = StopReason::Arrived;
        arrivedInSector = true;
    }

    if (worldStillValid && player)
    {
        ClearMovement();
        if (player->anictrl)
            player->anictrl->_Stand();
    }

    LastOutcome = Outcome();
    LastOutcome.Reason = reason;
    LastOutcome.Seconds = (NowMs() - StartedAt) / 1000.0f;
    LastOutcome.Recalculations = Recalculations;
    LastOutcome.BlockedLegs = static_cast<int>(NavBlocked.Count());
    LastOutcome.MetersLeft = worldStillValid && player ? Meters(RemainingLength(HeroPosition(), Path, Index)) : 0;
    LastOutcome.ArrivedInSector = arrivedInSector;
    ++Outcomes;

    YagaLog::Write(std::format("[nav] stop reason {} after {:.1f} s, {} recalculations, {} blocked, {} m left",
                               static_cast<int>(reason), LastOutcome.Seconds, Recalculations, NavBlocked.Count(),
                               LastOutcome.MetersLeft));

    switch (reason)
    {
    case StopReason::Arrived:
        Speech::Read(Loc::Format(L"Arrived, {}.", L"Jesteś na miejscu: {}.", DescribeTarget()));
        break;
    case StopReason::Cancelled:
        Speech::Read(Loc::T(L"Navigation cancelled.", L"Nawigacja przerwana."));
        break;
    case StopReason::Attacked:
        Speech::Read(Loc::T(L"Navigation stopped, you are under attack.", L"Nawigacja zatrzymana, ktoś cię atakuje."));
        break;
    case StopReason::NoRoute:
        Speech::Read(Loc::Format(L"No way through to {}.", L"Nie ma przejścia do: {}.", DescribeTarget()));
        break;
    case StopReason::GaveUp:
        Speech::Read(Loc::Format(L"Navigation stopped, cannot get through. {} left.",
                                 L"Nawigacja zatrzymana, nie da się przejść. Do celu {}.",
                                 Loc::Meters(LastOutcome.MetersLeft)));
        break;
    case StopReason::WorldChanged:
    case StopReason::Dead:
        break;
    }

    Path.clear();
    Index = 0;
    TargetWaypoint = nullptr;
    MeshRoute = false;
    LegKinds.clear();
    LegTags.clear();
    ReleaseWaypoints(MeshWaypoints);
}

// A navigation that could not even start still counts as finished, so a test tour sees its result.
void RecordFailedStart(const StopReason reason)
{
    LastOutcome = Outcome();
    LastOutcome.Reason = reason;
    ++Outcomes;
}

// Goals: every waypoint of the sector, nearest to its centre first; the waypoints nearest a point; or one waypoint.
std::vector<zCWaypoint *> CurrentGoals(zCWayNet *wayNet)
{
    switch (Kind)
    {
    case TargetKind::Sector:
    {
        std::vector<zCWaypoint *> goals = WaypointsInSector(wayNet, TargetSector);
        if (TargetPlaceWaypoint)
        {
            goals.erase(std::remove(goals.begin(), goals.end(), TargetPlaceWaypoint), goals.end());
            goals.insert(goals.begin(), TargetPlaceWaypoint);
        }
        return goals;
    }
    case TargetKind::Position:
        return WaypointsNear(wayNet, TargetPosition);
    case TargetKind::Waypoint:
        return RequestedWaypoint ? std::vector<zCWaypoint *>{RequestedWaypoint} : std::vector<zCWaypoint *>{};
    default:
        return {};
    }
}

// NavigatorMode 1 hands the route to the engine, which only knows way net ways and ignores blocked legs.
bool RouteFitsEngine(const std::vector<zCWaypoint *> &waypoints)
{
    if (Settings::NavigatorMode != 1 || NavBlocked.Legs.size() || NavBlocked.Starts.size())
        return false;
    for (size_t i = 0; i < waypoints.size(); ++i)
    {
        if (WaynetPatch::IsOurs(waypoints[i]))
            return false;
        if (i > 0 && (!waypoints[i - 1]->HasWay(waypoints[i]) || WaynetPatch::IsAvoided(waypoints[i - 1], waypoints[i])))
            return false;
    }
    return true;
}

// The navigation mesh when it is ready and the hero stands on it, otherwise (or when the mesh finds no way) the way
// net.
bool PlanAny(zCWayNet *wayNet, const zVEC3 &from, Plan &plan, Blocked &blocked)
{
    if (NavMesh::Ready() && PlanMeshRoute(from, plan, blocked))
        return true;
    if (!wayNet)
        return false;
    const std::vector<zCWaypoint *> goals = CurrentGoals(wayNet);
    if (goals.empty())
        return false;
    return PlanRoute(wayNet, from, goals, plan, blocked);
}

// Takes over a plan: its waypoints become the path, and the temporary waypoints of the previous mesh path are
// released.
void AdoptPlan(const Plan &plan)
{
    std::vector<zCWaypoint *> previous = std::move(MeshWaypoints);
    MeshWaypoints.clear();
    Path = plan.Waypoints;
    Index = 0;
    TargetWaypoint = plan.Target;
    MeshRoute = plan.Mesh;
    LegKinds = plan.Legs;
    LegTags = plan.Tags;
    if (plan.Mesh)
        MeshWaypoints = plan.Waypoints;
    UseEngineRoute = !plan.Mesh && RouteFitsEngine(Path);
    RetriesOnWaypoint = 0;
    ReleaseWaypoints(previous);
}

// Why a target cannot be reached, said when no route was found.
std::wstring NoRouteMessage()
{
    if (Kind == TargetKind::Sector)
    {
        zCWayNet *wayNet = GetWayNet();
        const bool waypoints = wayNet && !WaypointsInSector(wayNet, TargetSector).empty();
        if (NavMesh::Ready())
        {
            const std::vector<int> nodes = NavMesh::SectorNodes(TargetSector);
            if (nodes.empty() && !waypoints)
                return Loc::Format(L"{}: unreachable area, nothing to walk on.",
                                   L"{}: obszar niedostępny, nie ma tam gdzie chodzić.", DescribeTarget());
            const int heroIsland = NavMesh::IslandOf(NavMesh::NodeAt(HeroPosition()));
            if (heroIsland >= 0)
            {
                bool connected = false;
                for (const int node : nodes)
                    connected = connected || NavMesh::IslandOf(node) == heroIsland;
                if (!connected)
                    return Loc::Format(L"No way to {} from here.", L"Stąd nie da się dojść do: {}.",
                                       DescribeTarget());
            }
        }
        else if (!waypoints)
            return Loc::Format(L"No waypoints in {}, unreachable area.",
                               L"Brak punktów trasy w: {}, obszar niedostępny.", DescribeTarget());
    }
    return Loc::Format(L"No route to {}.", L"Brak trasy do: {}.", DescribeTarget());
}

bool Begin()
{
    zCWayNet *wayNet = GetWayNet();
    if (!player || (!wayNet && !NavMesh::Ready()))
        return false;

    if (Active)
    {
        ClearMovement();
        Active = false;
    }

    NavBlocked = Blocked();
    TriedDoors.clear();
    ReportedGates.clear();

    Plan plan;
    const bool found = PlanAny(wayNet, HeroPosition(), plan, NavBlocked);
    LogPlan(found ? "plan" : "plan failed", plan, NavBlocked);
    if (!found)
    {
        if (NavMesh::Building())
            YagaLog::Write("[nav] the navigation mesh is still being built");
        Speech::Read(NoRouteMessage());
        RecordFailedStart(StopReason::NoRoute);
        return false;
    }

    Active = true;
    AdoptPlan(plan);
    NavWorld = ogame->GetGameWorld();
    NavWayNet = wayNet;
    RetriesOnWaypoint = 0;
    Recalculations = 0;
    WaypointsSinceAnnouncement = 0;
    LastHitpoints = player->GetAttribute(NPC_ATR_HITPOINTS);
    StartedAt = NowMs();

    Speech::Read(Loc::Format(L"Walking to {}, {}, {}.", L"Prowadzę do: {}, {}, {}.", DescribeTarget(),
                             Loc::Meters(Meters(plan.Length)), Loc::Waypoints(static_cast<int>(plan.Waypoints.size()))));
    IssueStep();
    return true;
}

bool StartToSector(const SECTOR &sector)
{
    Kind = TargetKind::Sector;
    TargetSector = sector;
    RequestedWaypoint = nullptr;
    TargetPlaceName.clear();
    TargetPlaceWaypoint = nullptr;
    const std::vector<const Places::Entry *> places = Places::InSector(sector);
    if (!places.empty())
    {
        TargetPlaceName = Places::Name(*places.front());
        TargetPlaceWaypoint = places.front()->Waypoint;
    }
    return Begin();
}

bool StartToWaypoint(const std::string &name)
{
    zCWaypoint *wp = FindWaypoint(name);
    if (!wp)
    {
        Speech::Read(Loc::Format(L"Waypoint {} not found.", L"Nie ma punktu {}.", ToWString(name)));
        RecordFailedStart(StopReason::NoRoute);
        return false;
    }

    Kind = TargetKind::Waypoint;
    RequestedWaypoint = wp;
    return Begin();
}

bool StartToPosition(const zVEC3 &position)
{
    Kind = TargetKind::Position;
    TargetPosition = position;
    RequestedWaypoint = nullptr;
    return Begin();
}

// Plan again from where the hero stands, keeping the same destination and leaving out blocked legs.
void Recalculate(const std::wstring &why)
{
    ++Recalculations;
    if (Recalculations > MaxRecalculations)
    {
        Stop(StopReason::GaveUp);
        return;
    }

    zCWayNet *wayNet = GetWayNet();
    Plan plan;
    const bool found = PlanAny(wayNet, HeroPosition(), plan, NavBlocked);
    LogPlan(found ? "recalculated" : "recalculation failed", plan, NavBlocked);
    if (!found)
    {
        Stop(StopReason::NoRoute);
        return;
    }

    Speech::Read(Loc::Format(L"{}, recalculating. {}.", L"{}, przeliczam trasę. {}.", why,
                             Loc::Meters(Meters(plan.Length))));
    AdoptPlan(plan);
    IssueStep();
}

// The hero could not walk the current leg: remember it so the next plan goes around it.
void BlockCurrentLeg()
{
    if (MeshRoute)
    {
        // The hero could not swim out there: leave that bank out, so the next plan picks another way out.
        if (LegWet || HeroInWater())
        {
            const std::vector<int> bank = NavMesh::BankToBlock(Path[Index]->GetPositionWorld());
            for (const int node : bank)
                NavBlocked.Nodes.insert(node);
            YagaLog::Write(std::format("[nav] blocked {} polygons of the bank at {}", bank.size(),
                                       WaypointName(Path[Index])));
            if (!bank.empty())
                return;
        }
        // A ladder or way net way that did not work: leave that link out.
        if (Index < LegKinds.size() && LegKinds[Index] == NavCore::LegKind::Link)
        {
            NavBlocked.LinkTags.insert(LegTags[Index]);
            YagaLog::Write(std::format("[nav] blocked {} link {}",
                                       LegTags[Index] >= NavMesh::WayTagBase ? "way" : "ladder", LegTags[Index]));
            return;
        }
        // The polygons between the hero and the corner it could not reach.
        const std::vector<int> nodes = NavMesh::NodesToBlock(HeroPosition(), Path[Index]->GetPositionWorld());
        for (const int node : nodes)
            NavBlocked.Nodes.insert(node);
        YagaLog::Write(std::format("[nav] blocked {} mesh polygons toward {}", nodes.size(), WaypointName(Path[Index])));
        return;
    }

    if (Index > 0)
    {
        NavBlocked.Legs.insert(MakeLeg(Path[Index - 1], Path[Index]));
        YagaLog::Write(std::format("[nav] blocked leg {} - {}", WaypointName(Path[Index - 1]),
                                   WaypointName(Path[Index])));
    }
    else
    {
        NavBlocked.Starts.insert(Path[0]);
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

// A closed door near the hero, between it and the waypoint it is heading for, or right next to it. The door the way
// net puts on the current leg comes first.
oCMobDoor *FindClosedDoor()
{
    const zVEC3 hero = HeroPosition();
    if (Index > 0)
        if (oCMobDoor *door = DoorOnLeg(Path[Index - 1], Path[Index]))
            if (!TriedDoors.count(door) && door->GetState() == 0 &&
                hero.Distance(door->GetPositionWorld()) < DoorSearchRadius * 2.0f)
                return door;

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
        Speech::Read(Loc::T(L"Locked door.", L"Drzwi zamknięte na klucz."));
        YagaLog::Write("[nav] locked door " + std::string(door->GetObjectName().ToChar()));
        return false;
    }

    // Open it through the mob interaction: the engine's spot and heading, StartInteraction, then Interact (action and
    // forward) until the door is open. An EV_USEMOB message would be the NPC way, but the player's event manager never
    // finishes it.
    ClearMovement();
    Halt();
    door->SetIdealPosition(player);
    door->SetHeading(player);
    door->StartInteraction(player);

    DoorBeingOpened = door;
    CurrentPhase = Phase::OpeningDoor;
    PhaseStartedAt = NowMs();
    YagaLog::Write(std::format("[nav] opening door {} at {} m", door->GetObjectName().ToChar(),
                               Meters(HeroPosition().Distance(door->GetPositionWorld()))));
    return true;
}

// A gate (a moving vob such as a portcullis) on the current leg. The game opens gates with winches or by script; the
// navigator cannot tell whether one is open, so it is only named when the hero cannot get past it.
zCVob *GateOnLeg()
{
    const zVEC3 hero = HeroPosition();
    const zVEC3 from = Index > 0 ? Path[Index - 1]->GetPositionWorld() : hero;
    const zVEC3 to = Path[Index]->GetPositionWorld();

    zCArray<zCVob *> movers;
    ogame->GetGameWorld()->SearchVobListByBaseClass(zCMover::classDef, movers, nullptr);
    for (int i = 0; i < movers.GetNumInList(); ++i)
    {
        zCVob *gate = movers[i];
        if (!gate)
            continue;
        const zVEC3 position = gate->GetPositionWorld();
        if (hero.Distance(position) < 1200.0f && DistanceToLegXZ(position, from, to) < 400.0f)
            return gate;
    }
    return nullptr;
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

    Halt();
    std::wstring why = Loc::T(L"Blocked", L"Przeszkoda");
    if (zCVob *gate = GateOnLeg())
    {
        why = Loc::T(L"Gate closed", L"Brama zamknięta");
        if (ReportedGates.insert(gate).second)
            YagaLog::Write(std::format("[nav] closed gate {}", gate->GetObjectName().ToChar()));
    }
    BlockCurrentLeg();
    Recalculate(why);
}

void UpdateDriving(const zVEC3 &hero)
{
    zVEC3 target = Path[Index]->GetPositionWorld();
    target[VY] = hero[VY];

    // Never push a hero on dry land into deep water, unless the leg is meant to be swum.
    if (!HeroInWater() && !LegWet)
    {
        zVEC3 direction = target - hero;
        direction[VY] = 0.0f;
        if (direction.LengthApprox() > 1.0f)
        {
            const zVEC3 ahead = hero + direction.Normalize() * 150.0f;
            if (WaterDepthAt(ahead) > DeepWater)
            {
                Halt();
                YagaLog::Write("[nav] driving: deep water ahead");
                BlockCurrentLeg();
                Recalculate(Loc::T(L"Water ahead", L"Przed tobą woda"));
                return;
            }
        }
    }

    const float now = NowMs();
    oCAniCtrl_Human *ani = player->anictrl;
    const bool swimming = ani && ani->IsInWater();

    // Under water: a diving hero kept going forward sinks deeper. Stop so the hero floats up, then swim on.
    if (ani && ani->walkmode == ANI_WALKMODE_DIVE)
    {
        Halt();
        if (now - StuckReferenceTime > DriveGiveUpMs * 4.0f)
            OnStepFailed("under water");
        return;
    }

    player->SetHeadingYWorld(target);

    // A jump from standing climbs the ledge in front of the hero; one from running clears a gap. The engine's player
    // jump (PC_JumpForward) picks climb or jump by itself. Never in water, where a jump dives.
    const bool standStill = !swimming && DriveJumps > 0 && DriveClimbing && now < DriveJumpUntil + 800.0f;
    if (standStill)
        Halt();
    else
        RunForward();
    if (!swimming && DriveJumpStart > 0.0f && now >= DriveJumpStart)
    {
        DriveJumpStart = 0.0f;
        ani->PC_JumpForward();
    }

    // Progress means getting closer to the waypoint, not just moving: jumping on the spot moves the hero too.
    const zVEC3 goal = Path[Index]->GetPositionWorld();
    const float remaining = DistanceXZ(hero, goal) + std::abs(hero[VY] - goal[VY]);
    if (remaining < DriveBest - StuckMinMove)
    {
        DriveBest = remaining;
        ResetStuckWatch();
        DriveJumps = 0;
        return;
    }

    const float idle = now - StuckReferenceTime;
    if (swimming)
    {
        // Against the bank: the engine climbs out of the water when the ledge is low enough.
        if (idle > DriveJumpAfterMs && ani->Swim_CanClimbLedge())
        {
            YagaLog::Write("[nav] driving: climbing out of the water");
            ani->Swim_ClimbLedge();
            ResetStuckWatch();
            return;
        }
        if (idle > DriveGiveUpMs)
            OnStepFailed("swimming made no progress");
        return;
    }

    if (DriveJumps < 2 && idle > DriveJumpAfterMs * (DriveJumps + 1))
    {
        DriveClimbing = DriveJumps == 0;
        DriveJumpStart = now + (DriveClimbing ? 250.0f : 0.0f);
        DriveJumpUntil = DriveJumpStart + DriveJumpHoldMs;
        ++DriveJumps;
        YagaLog::Write(std::format("[nav] driving: no progress, {}", DriveClimbing ? "climbing" : "jumping"));
        return;
    }

    if (idle > DriveGiveUpMs)
        OnStepFailed("driving made no progress");
}

void AdvanceWaypoint();

void UpdateLadder(const zVEC3 &hero)
{
    const float elapsed = NowMs() - PhaseStartedAt;
    const bool onLadder = player->GetInteractMob() == LadderInUse;

    if (!LadderMounted)
    {
        if (onLadder)
        {
            LadderMounted = true;
            PhaseStartedAt = NowMs();
            Halt();
            YagaLog::Write("[nav] on the ladder");
            return;
        }

        // Walk up to it first.
        if (DistanceXZ(hero, LadderInUse->GetPositionWorld()) > LadderReachDistance)
        {
            zVEC3 look = LadderInUse->GetPositionWorld();
            look[VY] = hero[VY];
            player->SetHeadingYWorld(look);
            RunForward();
            if (elapsed > LadderMountMs + 4000.0f)
            {
                Halt();
                LadderInUse = nullptr;
                OnStepFailed("could not reach the ladder");
            }
            return;
        }

        Halt();
        LadderInUse->SetIdealPosition(player);
        LadderInUse->SetHeading(player);
        LadderInUse->StartInteraction(player);
        if (player->GetInteractMob() != LadderInUse && elapsed > LadderMountMs)
        {
            LadderInUse = nullptr;
            OnStepFailed("could not get on the ladder");
        }
        return;
    }

    if (onLadder)
    {
        static float lastReport = 0.0f;
        if (NowMs() - lastReport > 1000.0f)
        {
            lastReport = NowMs();
            YagaLog::Write(std::format("[nav] climbing: state {}/{}, height {:.0f}", LadderInUse->state,
                                       LadderInUse->state_num, hero[VY]));
        }
        LadderInUse->Interact(player, TRUE, LadderUp ? TRUE : FALSE, LadderUp ? FALSE : TRUE, FALSE, FALSE);
        if (elapsed > LadderClimbMs)
        {
            LadderInUse->StopInteraction(player);
            LadderInUse = nullptr;
            OnStepFailed("stuck on the ladder");
        }
        return;
    }

    // Off the ladder at the other end.
    YagaLog::Write(std::format("[nav] off the ladder after {:.1f} s, {} m from {}", elapsed / 1000.0f,
                               Meters(DistanceXZ(hero, Path[Index]->GetPositionWorld())), WaypointName(Path[Index])));
    LadderInUse = nullptr;
    if (Within(hero, Path[Index], EngineDoneRadius))
        AdvanceWaypoint();
    else
    {
        CurrentPhase = Phase::Walking;
        oCMsgMovement *message = new oCMsgMovement(oCMsgMovement::EV_GOTOPOS, Path[Index]->GetPositionWorld());
        player->GetEM()->OnMessage(message, player);
        ResetStuckWatch();
    }
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
        Speech::Read(Loc::Meters(Meters(RemainingLength(HeroPosition(), Path, Index))) + L".");
    }

    if (!UseEngineRoute)
        IssueStep();
}

bool PlayerWantsControl()
{
    static const unsigned short movementKeys[] = {GAME_UP,    GAME_DOWN,       GAME_LEFT,
                                                  GAME_RIGHT, GAME_STRAFELEFT, GAME_STRAFERIGHT};
    for (const unsigned short key : movementKeys)
        if (zinput->GetState(key) > 0.0f)
            return true;
    return false;
}

void SpeakStatus()
{
    if (!Active)
    {
        Speech::Read(Loc::T(L"Navigator idle.", L"Nawigator nieaktywny."));
        return;
    }

    Speech::Read(Loc::Format(L"Walking to {}, {} left, waypoint {} of {}.", L"Prowadzę do: {}, do celu {}, punkt {} z {}.",
                             DescribeTarget(), Loc::Meters(Meters(RemainingLength(HeroPosition(), Path, Index))),
                             Index + 1, Path.size()));
}

void UpdateEngineRoute(const zVEC3 &hero, const bool engineBusy)
{
    while (Index + 1 < Path.size() && Within(hero, Path[Index], ArriveRadius * 2.0f))
        AdvanceWaypoint();

    if (!engineBusy)
    {
        if (Within(hero, Path.back(), EngineDoneRadius))
            Stop(StopReason::Arrived);
        else
            Recalculate(Loc::T(L"Route interrupted", L"Trasa przerwana"));
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
        const bool open = DoorBeingOpened && DoorBeingOpened->GetState() != 0;
        const bool interacting = DoorBeingOpened && player->GetInteractMob() == DoorBeingOpened;
        if (!open && interacting)
            DoorBeingOpened->Interact(player, TRUE, TRUE, FALSE, FALSE, FALSE);

        if (open || elapsed > DoorTimeoutMs)
        {
            if (interacting)
                DoorBeingOpened->StopInteraction(player);
            YagaLog::Write(std::format("[nav] door {} after {:.1f} s", open ? "open" : "still closed",
                                       elapsed / 1000.0f));
            if (open)
                Speech::Read(Loc::T(L"Door opened.", L"Drzwi otwarte."), 0);
            DoorBeingOpened = nullptr;
            IssueStep();
        }
        return;
    }

    if (CurrentPhase == Phase::Ladder)
    {
        UpdateLadder(hero);
        return;
    }

    if (UseEngineRoute)
    {
        UpdateEngineRoute(hero, engineBusy);
        return;
    }

    if ((CurrentPhase == Phase::Driving || CurrentPhase == Phase::Walking) && NowMs() - StepStartedAt > StepLimitMs)
    {
        OnStepFailed("the leg took too long");
        return;
    }

    if (CurrentPhase == Phase::Driving)
    {
        if (Within(hero, Path[Index], ArriveRadius))
        {
            Halt();
            AdvanceWaypoint();
        }
        else
            UpdateDriving(hero);
        return;
    }

    if (Within(hero, Path[Index], ArriveRadius) || (!engineBusy && Within(hero, Path[Index], EngineDoneRadius)))
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
        Recalculate(Loc::T(L"Off route", L"Zejście z trasy"));
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

// The game is paused (menu): stop the run; Update starts it again once the game runs.
void OnPause()
{
    Halt();
}

void Reset()
{
    Active = false;
    Halt();
    DoorBeingOpened = nullptr;
    LadderInUse = nullptr;
    WetLegs.clear();
    CurrentPhase = Phase::Walking;
    Path.clear();
    Index = 0;
    TargetWaypoint = nullptr;
    NavWorld = nullptr;
    NavWayNet = nullptr;
    NavBlocked = Blocked();
    TriedDoors.clear();
    Kind = TargetKind::None;
    RequestedWaypoint = nullptr;
    TargetPlaceName.clear();
    TargetPlaceWaypoint = nullptr;
    UseEngineRoute = false;
    MeshRoute = false;
    LegKinds.clear();
    LegTags.clear();
    ReleaseWaypoints(MeshWaypoints);
    // Patch nodes belong to the old world's way net.
    WaynetPatch::Clear();
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
