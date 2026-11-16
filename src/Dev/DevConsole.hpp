#include <fstream>
#include <map>
#include <random>
#include <sstream>

namespace GOTHIC_NAMESPACE
{
// Test hook for development: with [YAGA] DevCommands=1 the plugin polls yaga_cmd.txt next to the executable, runs
// each line as a command on the game thread, deletes the file and writes the results to YAGA.log.
namespace DevConsole
{
inline ULONGLONG NextPollAt = 0;

void Out(const std::string &line)
{
    YagaLog::Write("[dev] " + line);
}

void CommandPos()
{
    const zVEC3 position = player->GetPositionWorld();
    const SECTOR sector = SectorOf(position);
    zCWayNet *wayNet = Navigator::GetWayNet();
    zCWaypoint *nearest = wayNet ? wayNet->GetNearestWaypoint(position) : nullptr;

    Out(std::format("pos {:.0f} {:.0f} {:.0f} sector {} {} heading {:.0f} hp {}/{} nearest {} ({:.0f} cm, dy {:.0f})",
                    position[VX], position[VY], position[VZ], sector.x, sector.y, GetCompassAngle(),
                    player->GetAttribute(NPC_ATR_HITPOINTS), player->GetAttribute(NPC_ATR_HITPOINTSMAX),
                    Navigator::WaypointName(nearest), nearest ? position.Distance(nearest->GetPositionWorld()) : 0.0f,
                    nearest ? nearest->GetPositionWorld()[VY] - position[VY] : 0.0f));
}

// Way net statistics: waypoint and way counts plus connected components, the reason some sectors cannot be reached.
void CommandWayNetInfo()
{
    zCWayNet *wayNet = Navigator::GetWayNet();
    if (!wayNet)
    {
        Out("no way net");
        return;
    }

    std::vector<zCWaypoint *> all;
    Navigator::ForEachWaypoint(wayNet, [&](zCWaypoint *wp) { all.push_back(wp); });

    int ways = 0;
    int jumpWays = 0;
    int chasmWays = 0;
    for (auto node = wayNet->waylist.next; node; node = node->next)
    {
        if (!node->data)
            continue;
        ++ways;
        if (node->data->jump)
            ++jumpWays;
        if (node->data->chasm)
            ++chasmWays;
    }

    std::unordered_map<zCWaypoint *, int> component;
    std::vector<int> sizes;
    for (zCWaypoint *seed : all)
    {
        if (component.count(seed))
            continue;

        const int id = static_cast<int>(sizes.size());
        int size = 0;
        std::vector<zCWaypoint *> stack = {seed};
        component[seed] = id;
        while (!stack.empty())
        {
            zCWaypoint *wp = stack.back();
            stack.pop_back();
            ++size;
            for (auto node = wp->wayList.next; node; node = node->next)
            {
                zCWaypoint *next = node->data ? Navigator::OtherEnd(node->data, wp) : nullptr;
                if (next && !component.count(next))
                {
                    component[next] = id;
                    stack.push_back(next);
                }
            }
        }
        sizes.push_back(size);
    }

    std::vector<int> sorted = sizes;
    std::sort(sorted.rbegin(), sorted.rend());
    std::string top;
    for (size_t i = 0; i < sorted.size() && i < 8; ++i)
        top += std::format("{} ", sorted[i]);

    zCWaypoint *nearest = wayNet->GetNearestWaypoint(player->GetPositionWorld());
    Out(std::format("waynet {} waypoints, {} ways ({} jump, {} chasm), {} components, largest: {}; hero component {}",
                    all.size(), ways, jumpWays, chasmWays, sizes.size(), top,
                    nearest && component.count(nearest) ? sizes[component[nearest]] : -1));
}

void CommandRoute(const SECTOR &sector)
{
    zCWayNet *wayNet = Navigator::GetWayNet();
    Navigator::Plan plan;
    const bool found =
        Navigator::PlanRoute(wayNet, player->GetPositionWorld(), Navigator::WaypointsInSector(wayNet, sector), plan);
    Navigator::LogPlan(found ? "route" : "route not found", plan);
}

// Times the engine's A* between random waypoint pairs, to see what route planning actually costs.
void CommandBench(const int count)
{
    zCWayNet *wayNet = Navigator::GetWayNet();
    std::vector<zCWaypoint *> all;
    Navigator::ForEachWaypoint(wayNet, [&](zCWaypoint *wp) { all.push_back(wp); });
    if (all.size() < 2)
        return;

    std::mt19937 random(12345);
    std::uniform_int_distribution<size_t> pick(0, all.size() - 1);

    int found = 0;
    double total = 0.0;
    double worst = 0.0;
    size_t longest = 0;
    for (int i = 0; i < count; ++i)
    {
        zCWaypoint *from = all[pick(random)];
        zCWaypoint *to = all[pick(random)];

        const auto started = std::chrono::high_resolution_clock::now();
        zCRoute *route = wayNet->FindRoute(from, to, player);
        const double ms =
            std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - started).count();

        total += ms;
        worst = (std::max)(worst, ms);
        if (route)
        {
            ++found;
            longest = (std::max)(longest, Navigator::ExtractWaypoints(route).size());
            delete route;
        }
    }

    Out(std::format("bench {} searches over {} waypoints: {} found, avg {:.3f} ms, worst {:.3f} ms, longest {} wps",
                    count, all.size(), found, total / count, worst, longest));
}

void CommandSectors()
{
    zCWayNet *wayNet = Navigator::GetWayNet();
    std::map<std::pair<LONG, LONG>, int> counts;
    Navigator::ForEachWaypoint(wayNet, [&](zCWaypoint *wp) {
        const SECTOR sector = SectorOf(wp->GetPositionWorld());
        ++counts[{sector.x, sector.y}];
    });

    std::string line;
    for (const auto &[key, count] : counts)
        line += std::format("({},{}):{} ", key.first, key.second, count);
    Out(std::format("{} sectors with waypoints: {}", counts.size(), line));
}

void CommandNear(const float radius)
{
    const zVEC3 hero = player->GetPositionWorld();
    int listed = 0;
    for (zCListSort<zCVob> *node = ogame->GetGameWorld()->voblist; node; node = node->next)
    {
        zCVob *vob = node->data;
        oCMobInter *mob = zDYNAMIC_CAST<oCMobInter>(vob);
        oCNpc *npc = zDYNAMIC_CAST<oCNpc>(vob);
        if ((!mob && !npc) || vob == player)
            continue;

        const float distance = hero.Distance(vob->GetPositionWorld());
        if (distance > radius)
            continue;

        if (mob)
        {
            oCMobDoor *door = zDYNAMIC_CAST<oCMobDoor>(vob);
            Out(std::format("  {} {} '{}' scheme {} {:.0f} cm state {} locked {}",
                            door ? (Navigator::IsDoor(door) ? "door" : "door-class") : "mob",
                            vob->GetObjectName().ToChar(), mob->GetName().ToChar(), mob->sceme.ToChar(), distance,
                            mob->GetState(), door ? static_cast<int>(door->locked) : -1));
        }
        else
            Out(std::format("  npc '{}' {:.0f} cm", npc->GetName(0).ToChar(), distance));
        ++listed;
    }
    Out(std::format("near: {} objects within {:.0f} cm", listed, radius));
}

void CommandDiag()
{
    oCAniCtrl_Human *ani = player->anictrl;
    zCEventManager *em = player->GetEM();
    std::string messages;
    for (int i = 0; i < em->GetNumMessages(); ++i)
    {
        zCEventMessage *message = em->GetEventMessage(i);
        messages += std::format("{}:{} ", message->_GetClassDef()->className.ToChar(), message->subType);
    }
    Out(std::format("diag walkmode {} wmode {} inWater {} standing {} em [{}] nav {} phase {} index {}/{}",
                    ani ? ani->walkmode : -1, ani ? ani->wmode : -1, ani ? ani->IsInWater() : -1,
                    ani ? ani->IsStanding() : -1, messages, Navigator::Active,
                    static_cast<int>(Navigator::CurrentPhase), Navigator::Index, Navigator::Path.size()));
}

inline ULONGLONG PushUntil = 0;

void Run(const std::string &line)
{
    std::istringstream in(line);
    std::string command;
    in >> command;
    if (command.empty() || command[0] == '#')
        return;

    Out("> " + line);

    if (command == "pos")
        CommandPos();
    else if (command == "wpinfo")
        CommandWayNetInfo();
    else if (command == "sectors")
        CommandSectors();
    else if (command == "route" || command == "nav")
    {
        SECTOR sector{};
        if (!(in >> sector.x >> sector.y))
        {
            Out("usage: " + command + " <x> <y>");
            return;
        }
        if (command == "route")
            CommandRoute(sector);
        else
            Navigator::StartToSector(sector);
    }
    else if (command == "navwp")
    {
        std::string name;
        in >> name;
        Navigator::StartToWaypoint(zSTRING(name.c_str()));
    }
    else if (command == "stop")
        Navigator::Stop(Navigator::StopReason::Cancelled);
    else if (command == "status")
        Navigator::SpeakStatus();
    else if (command == "mode")
    {
        in >> Settings::NavigatorMode;
        Out(std::format("navigator mode {}", Settings::NavigatorMode));
    }
    else if (command == "beam")
    {
        std::string name;
        in >> name;
        Navigator::Stop(Navigator::StopReason::Cancelled);
        player->BeamTo(zSTRING(name.c_str()));
        CommandPos();
    }
    else if (command == "near")
    {
        float radius = 800.0f;
        in >> radius;
        CommandNear(radius);
    }
    else if (command == "diag")
        CommandDiag();
    else if (command == "facedoor")
    {
        oCMobDoor *best = nullptr;
        float bestDistance = 1500.0f;
        const zVEC3 hero = player->GetPositionWorld();
        for (zCListSort<zCVob> *node = ogame->GetGameWorld()->voblist; node; node = node->next)
        {
            oCMobDoor *door = zDYNAMIC_CAST<oCMobDoor>(node->data);
            if (door && Navigator::IsDoor(door) && hero.Distance(door->GetPositionWorld()) < bestDistance)
            {
                best = door;
                bestDistance = hero.Distance(door->GetPositionWorld());
            }
        }
        if (best)
        {
            zVEC3 look = best->GetPositionWorld();
            look[VY] = hero[VY];
            player->SetHeadingYWorld(look);
            Out(std::format("facing door {} at {:.0f} cm, state {}", best->GetObjectName().ToChar(), bestDistance,
                            best->GetState()));
        }
        else
            Out("no door nearby");
    }
    else if (command == "water")
    {
        const zVEC3 hero = player->GetPositionWorld();
        zVEC3 at = player->GetAtVectorWorld();
        at[VY] = 0.0f;
        std::string line = std::format("water depth here {:.0f}", Navigator::WaterDepthAt(hero));
        for (const float metres : {1.5f, 3.0f, 6.0f, 10.0f})
            line += std::format(", {:.1f} m ahead {:.0f}", metres,
                                Navigator::WaterDepthAt(hero + at.Normalize() * (metres * 100.0f)));
        Out(line);
    }
    else if (command == "wp")
    {
        std::string name;
        in >> name;
        zCWaypoint *wp = Navigator::GetWayNet()->GetWaypoint(zSTRING(name.c_str()));
        if (!wp)
        {
            Out("no such waypoint");
            return;
        }
        std::string ways;
        for (auto node = wp->wayList.next; node; node = node->next)
            if (node->data)
                ways += Navigator::WaypointName(Navigator::OtherEnd(node->data, wp)) + " ";
        const zVEC3 position = wp->GetPositionWorld();
        Out(std::format("wp {} at {:.0f} {:.0f} {:.0f} waterDepth {} underWater {} ways: {}", name, position[VX],
                        position[VY], position[VZ], wp->waterDepth, wp->underWater, ways));
    }
    else if (command == "push")
    {
        int ms = 1000;
        in >> ms;
        zinput->SetState(GAME_UP, TRUE);
        PushUntil = GetTickCount64() + ms;
        Out(std::format("pushing forward for {} ms", ms));
    }
    else if (command == "bench")
    {
        int count = 200;
        in >> count;
        CommandBench((std::max)(1, count));
    }
    else
        Out("unknown command");
}

void Poll()
{
    if (!Settings::DevCommands || !player)
        return;

    const ULONGLONG now = GetTickCount64();
    if (PushUntil)
    {
        if (now < PushUntil)
            zinput->SetState(GAME_UP, TRUE);
        else
        {
            zinput->SetState(GAME_UP, FALSE);
            PushUntil = 0;
            Out("push done");
            CommandPos();
        }
    }

    if (now < NextPollAt)
        return;
    NextPollAt = now + 250;

    std::ifstream file("yaga_cmd.txt");
    if (!file)
        return;

    std::vector<std::string> lines;
    for (std::string line; std::getline(file, line);)
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
    }
    file.close();
    DeleteFileA("yaga_cmd.txt");

    for (const std::string &line : lines)
        Run(line);
}
} // namespace DevConsole
} // namespace GOTHIC_NAMESPACE
