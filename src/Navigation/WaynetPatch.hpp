#include <fstream>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace GOTHIC_NAMESPACE
{
// Extra waypoints and ways for the navigator only.
//
// The game's way net was made for NPCs, so some places the player can walk to have no waypoints (or only a separate
// patch of them). The patch adds nodes and links on top of the way net without touching it: the nodes are standalone
// zCWaypoint objects that are never inserted into zCWayNet, and links live in a map here instead of zCWay objects
// (a zCWay adds itself to the wayList of both ends, where the engine's A* would find it). NPC routes and savegames
// therefore stay exactly as they were.
//
// Format, one entry per line ('#' starts a comment):
//   world WORLD.ZEN        following entries apply to this world file only
//   node NAME x y z        a new waypoint; y may be rough, the node is dropped onto the ground below y + 1 m
//   link A B [jump]        a way between two nodes or way net waypoints; "jump" marks a leg that needs a jump
//   unlink A B             removes a link given earlier
//   avoid A B              a way net way the hero cannot walk (NPCs can), left out of routes
//
// Sources, in this order: the table compiled into the plugin (WaynetPatch_G1.hpp), then yaga_waynet_patch.txt in
// the game folder, so a player or the author can add a path without rebuilding.
namespace WaynetPatch
{
using Leg = std::pair<zCWaypoint *, zCWaypoint *>;

inline std::vector<zCWaypoint *> Nodes;
inline std::unordered_map<std::string, zCWaypoint *> ByName; // upper-case names of our nodes
inline std::unordered_map<zCWaypoint *, std::vector<zCWaypoint *>> Links;
inline std::set<Leg> Avoided;
inline std::set<Leg> JumpLegs;
inline zCWayNet *BuiltFor = nullptr;
inline std::string BuiltWorld;
inline int Problems = 0;
inline int Generation = 0; // bumped on every build, so holders of node pointers know to look them up again

inline Leg MakeLeg(zCWaypoint *a, zCWaypoint *b)
{
    return a < b ? Leg(a, b) : Leg(b, a);
}

inline std::string Upper(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::toupper(c); });
    return text;
}

// "C:\...\WORLD.ZEN" or "WORLD.ZEN" -> "WORLD.ZEN"
inline std::string WorldFileName(oCWorld *world)
{
    std::string name = world ? world->worldFilename.ToChar() : "";
    const size_t slash = name.find_last_of("\\/");
    if (slash != std::string::npos)
        name = name.substr(slash + 1);
    return Upper(name);
}

// The game folder: the executable lives in <game>\system.
inline std::string GameFolder()
{
    char path[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string folder = path;
    for (int i = 0; i < 2; ++i)
    {
        const size_t slash = folder.find_last_of("\\/");
        if (slash == std::string::npos)
            break;
        folder.resize(slash);
    }
    return folder;
}

inline std::string PatchFilePath()
{
    return GameFolder() + "\\yaga_waynet_patch.txt";
}

inline bool IsOurs(zCWaypoint *wp)
{
    for (zCWaypoint *node : Nodes)
        if (node == wp)
            return true;
    return false;
}

inline bool IsAvoided(zCWaypoint *a, zCWaypoint *b)
{
    return !Avoided.empty() && Avoided.count(MakeLeg(a, b)) != 0;
}

inline bool IsJumpLeg(zCWaypoint *a, zCWaypoint *b)
{
    return !JumpLegs.empty() && JumpLegs.count(MakeLeg(a, b)) != 0;
}

template <typename Fn> void ForEachLink(zCWaypoint *wp, Fn fn)
{
    const auto found = Links.find(wp);
    if (found == Links.end())
        return;
    for (zCWaypoint *next : found->second)
        fn(next);
}

inline void Clear()
{
    for (zCWaypoint *wp : Nodes)
        wp->Release(); // never Free(): that deletes ways and the waypoint vob
    Nodes.clear();
    ByName.clear();
    Links.clear();
    Avoided.clear();
    JumpLegs.clear();
    BuiltFor = nullptr;
    BuiltWorld.clear();
    Problems = 0;
}

// Our node first, then the way net.
inline zCWaypoint *Find(zCWayNet *wayNet, const std::string &name)
{
    const std::string upper = Upper(name);
    const auto found = ByName.find(upper);
    if (found != ByName.end())
        return found->second;
    return wayNet ? wayNet->GetWaypoint(zSTRING(upper.c_str())) : nullptr;
}

inline void AddLink(zCWaypoint *a, zCWaypoint *b)
{
    auto &fromA = Links[a];
    if (std::find(fromA.begin(), fromA.end(), b) == fromA.end())
        fromA.push_back(b);
    auto &fromB = Links[b];
    if (std::find(fromB.begin(), fromB.end(), a) == fromB.end())
        fromB.push_back(a);
}

inline void RemoveLink(zCWaypoint *a, zCWaypoint *b)
{
    auto erase = [](std::vector<zCWaypoint *> &list, zCWaypoint *wp) {
        list.erase(std::remove(list.begin(), list.end(), wp), list.end());
    };
    if (Links.count(a))
        erase(Links[a], b);
    if (Links.count(b))
        erase(Links[b], a);
    JumpLegs.erase(MakeLeg(a, b));
}

inline void Report(const std::string &source, const int line, const std::string &problem)
{
    ++Problems;
    YagaLog::Write(std::format("[patch] {}:{}: {}", source, line, problem));
}

inline void Apply(oCWorld *world, zCWayNet *wayNet, const std::string &worldName, std::istream &in,
                  const std::string &source)
{
    bool active = true; // entries before any "world" line apply to every world
    int lineNumber = 0;
    for (std::string line; std::getline(in, line);)
    {
        ++lineNumber;
        const size_t comment = line.find('#');
        if (comment != std::string::npos)
            line.resize(comment);

        std::istringstream words(line);
        std::string command;
        if (!(words >> command))
            continue;
        command = Upper(command);

        if (command == "WORLD")
        {
            std::string name;
            words >> name;
            active = Upper(name) == worldName;
            continue;
        }
        if (!active)
            continue;

        if (command == "NODE")
        {
            std::string name;
            float x = 0.0f, y = 0.0f, z = 0.0f;
            if (!(words >> name >> x >> y >> z))
            {
                Report(source, lineNumber, "usage: node NAME x y z");
                continue;
            }

            name = Upper(name);
            zCWaypoint *wp = nullptr;
            const auto existing = ByName.find(name);
            if (existing != ByName.end())
                wp = existing->second; // a later definition moves the node
            else if (wayNet->GetWaypoint(zSTRING(name.c_str())))
            {
                Report(source, lineNumber, name + " is already a way net waypoint");
                continue;
            }
            else
            {
                wp = new zCWaypoint();
                zSTRING zname(name.c_str());
                wp->SetName(zname);
                Nodes.push_back(wp);
                ByName[name] = wp;
            }

            zVEC3 position(x, y + 100.0f, z);
            wp->Init(position);
            wp->CorrectHeight(world); // traces 10 m down, puts the node 50 cm above the ground like the way net
            continue;
        }

        if (command == "LINK" || command == "UNLINK" || command == "AVOID")
        {
            std::string nameA, nameB, flag;
            if (!(words >> nameA >> nameB))
            {
                Report(source, lineNumber, "usage: " + command + " A B");
                continue;
            }
            words >> flag;

            zCWaypoint *a = Find(wayNet, nameA);
            zCWaypoint *b = Find(wayNet, nameB);
            if (!a || !b || a == b)
            {
                Report(source, lineNumber, std::format("unknown waypoint in {} {} {}", command, nameA, nameB));
                continue;
            }

            if (command == "LINK")
            {
                AddLink(a, b);
                if (Upper(flag) == "JUMP")
                    JumpLegs.insert(MakeLeg(a, b));
            }
            else if (command == "UNLINK")
                RemoveLink(a, b);
            else
            {
                if (!a->HasWay(b))
                    Report(source, lineNumber, std::format("avoid {} {}: no such way in the way net", nameA, nameB));
                Avoided.insert(MakeLeg(a, b));
            }
            continue;
        }

        Report(source, lineNumber, "unknown entry " + command);
    }
}

// Builds the patch for the current world, once per way net.
inline void EnsureBuilt()
{
    oCWorld *world = ogame ? ogame->GetGameWorld() : nullptr;
    zCWayNet *wayNet = world ? world->wayNet : nullptr;
    if (!wayNet)
    {
        if (BuiltFor)
            Clear();
        return;
    }

    const std::string worldName = WorldFileName(world);
    if (wayNet == BuiltFor && worldName == BuiltWorld)
        return;

    Clear();
    BuiltFor = wayNet;
    BuiltWorld = worldName;
    ++Generation;

    std::istringstream compiled(CompiledPatch());
    Apply(world, wayNet, worldName, compiled, "built-in");

    std::ifstream file(PatchFilePath());
    if (file)
        Apply(world, wayNet, worldName, file, "yaga_waynet_patch.txt");

    size_t links = 0;
    for (const auto &[wp, list] : Links)
        links += list.size();
    YagaLog::Write(std::format("[patch] world {}: {} nodes, {} links, {} avoided ways, {} problems{}", worldName,
                               Nodes.size(), links / 2, Avoided.size(), Problems,
                               file ? ", with yaga_waynet_patch.txt" : ""));
}

// Drop the patch so the next EnsureBuilt reads it again (the file may have changed).
inline void Reload()
{
    Clear();
    EnsureBuilt();
}
} // namespace WaynetPatch
} // namespace GOTHIC_NAMESPACE
