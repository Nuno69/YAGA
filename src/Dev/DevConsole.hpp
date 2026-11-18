#include <fstream>
#include <map>
#include <random>
#include <sstream>

namespace GOTHIC_NAMESPACE
{
// Test hook for development: with [YAGA] DevCommands=1 the plugin polls yaga_cmd.txt next to the executable, runs
// each line as a command on the game thread, deletes the file and writes the results to YAGA.log.
//
// Coordinates are world units (cm), like the "pos" output.
namespace DevConsole
{
inline ULONGLONG NextPollAt = 0;

void Out(const std::string &line)
{
    YagaLog::Write("[dev] " + line);
}

float GameMs()
{
    return Navigator::NowMs();
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

// Connected components of the way net plus the patch; returns the component id per waypoint and the sizes.
void Components(zCWayNet *wayNet, std::unordered_map<zCWaypoint *, int> &component, std::vector<int> &sizes)
{
    std::vector<zCWaypoint *> all;
    Navigator::ForEachWaypoint(wayNet, [&](zCWaypoint *wp) { all.push_back(wp); });

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
            Navigator::ForEachNeighbour(wp, [&](zCWaypoint *next) {
                if (!component.count(next))
                {
                    component[next] = id;
                    stack.push_back(next);
                }
            });
        }
        sizes.push_back(size);
    }
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

    int waypoints = 0;
    Navigator::ForEachWaypoint(wayNet, [&](zCWaypoint *) { ++waypoints; });

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
    Components(wayNet, component, sizes);

    std::vector<int> sorted = sizes;
    std::sort(sorted.rbegin(), sorted.rend());
    std::string top;
    for (size_t i = 0; i < sorted.size() && i < 8; ++i)
        top += std::format("{} ", sorted[i]);

    zCWaypoint *nearest = wayNet->GetNearestWaypoint(player->GetPositionWorld());
    Out(std::format("waynet {} waypoints ({} patch nodes), {} ways ({} jump, {} chasm), {} components, largest: {}; "
                    "hero component {}",
                    waypoints, WaynetPatch::Nodes.size(), ways, jumpWays, chasmWays, sizes.size(), top,
                    nearest && component.count(nearest) ? sizes[component[nearest]] : -1));
}

void CommandRoute(const SECTOR &sector)
{
    zCWayNet *wayNet = Navigator::GetWayNet();
    Navigator::Plan plan;
    Navigator::Blocked blocked;
    const bool found = Navigator::PlanRoute(wayNet, player->GetPositionWorld(),
                                            Navigator::WaypointsInSector(wayNet, sector), plan, blocked);
    Navigator::LogPlan(found ? "route" : "route not found", plan, blocked);
}

// The navigation mesh: which sectors have something to walk on, which of them the hero's island reaches, the other
// islands, and whether each named place is on the hero's island.
void CommandRouteAllMesh()
{
    const NavCore::Graph &graph = NavMesh::Current->graph;
    const int heroIsland = NavMesh::IslandOf(NavMesh::NodeAt(player->GetPositionWorld()));

    std::map<std::pair<LONG, LONG>, std::pair<int, int>> sectors; // polygons, polygons on the hero's island
    std::vector<int> islandSize(graph.IslandCount(), 0);
    std::vector<zVEC3> islandSum(graph.IslandCount(), zVEC3(0.0f, 0.0f, 0.0f));
    for (int node = 0; node < graph.Count(); ++node)
    {
        const float *c = graph.Center(node);
        const zVEC3 center(c[0], c[1], c[2]);
        const SECTOR sector = SectorOf(center);
        auto &entry = sectors[{sector.x, sector.y}];
        ++entry.first;
        const int island = graph.Island(node);
        if (island == heroIsland)
            ++entry.second;
        if (island >= 0)
        {
            ++islandSize[island];
            islandSum[island] += center;
        }
    }

    std::string unreachable;
    int reached = 0;
    for (const auto &[key, counts] : sectors)
    {
        if (counts.second)
            ++reached;
        else
            unreachable += std::format("({},{}):{} ", key.first, key.second, counts.first);
    }
    Out(std::format("routeall mesh: hero island {} with {} polygons; {} sectors with walkable surface, {} reachable",
                    heroIsland, heroIsland >= 0 ? graph.IslandSize(heroIsland) : 0, sectors.size(), reached));
    Out("routeall mesh unreachable sectors: " + unreachable);

    std::vector<int> order;
    for (int i = 0; i < graph.IslandCount(); ++i)
        if (i != heroIsland)
            order.push_back(i);
    std::sort(order.begin(), order.end(), [&](const int a, const int b) { return islandSize[a] > islandSize[b]; });
    for (size_t i = 0; i < order.size() && i < 20; ++i)
    {
        const int island = order[i];
        const zVEC3 centre = islandSum[island] * (1.0f / static_cast<float>((std::max)(1, islandSize[island])));
        const SECTOR sector = SectorOf(centre);
        Out(std::format("routeall mesh island {}: {} polygons around {:.0f} {:.0f} {:.0f} (sector {} {})", island,
                        islandSize[island], centre[VX], centre[VY], centre[VZ], sector.x, sector.y));
    }

    // Sectors the way net reaches but the mesh does not: places the mesh may be missing.
    if (zCWayNet *wayNet = Navigator::GetWayNet())
    {
        std::map<std::pair<LONG, LONG>, int> missed;
        Navigator::ForEachWaypoint(wayNet, [&](zCWaypoint *wp) {
            const SECTOR sector = SectorOf(wp->GetPositionWorld());
            const auto found = sectors.find({sector.x, sector.y});
            if (found == sectors.end() || found->second.second == 0)
                ++missed[{sector.x, sector.y}];
        });
        std::string list;
        for (const auto &[key, count] : missed)
            list += std::format("({},{}):{} ", key.first, key.second, count);
        Out(std::format("routeall mesh: {} sectors with waypoints but no reachable mesh: {}", missed.size(), list));
    }

    Places::EnsureResolved();
    int placesOk = 0;
    for (const Places::Entry &entry : Places::Entries)
    {
        const int node = NavMesh::NodeAt(entry.Waypoint->GetPositionWorld());
        const bool ok = node >= 0 && graph.Island(node) == heroIsland;
        placesOk += ok ? 1 : 0;
        Out(std::format("routeall mesh place {} {}: polygon {} island {}", ok ? "OK" : "CUT", entry.Info->Waypoint,
                        node, node >= 0 ? graph.Island(node) : -1));
    }
    Out(std::format("routeall mesh places: {} of {} on the hero's island", placesOk, Places::Entries.size()));
}

// The static world mesh as the collector sees it, and the polygon under the hero as the engine's trace finds it.
void CommandNavGeometry()
{
    oCWorld *world = ogame->GetGameWorld();
    zCMesh *mesh = world->bspTree.mesh;
    const zVEC3 hero = player->GetPositionWorld();
    Out(std::format("navgeo: hero {:.0f} {:.0f} {:.0f}; mesh {} numPoly {} numVert {} bbox {:.0f} {:.0f} {:.0f} to "
                    "{:.0f} {:.0f} {:.0f}; bsp numPolys {} mode {}",
                    hero[VX], hero[VY], hero[VZ], static_cast<void *>(mesh), mesh ? mesh->numPoly : -1,
                    mesh ? mesh->numVert : -1, mesh ? mesh->bbox3D.mins[VX] : 0.0f, mesh ? mesh->bbox3D.mins[VY] : 0.0f,
                    mesh ? mesh->bbox3D.mins[VZ] : 0.0f, mesh ? mesh->bbox3D.maxs[VX] : 0.0f,
                    mesh ? mesh->bbox3D.maxs[VY] : 0.0f, mesh ? mesh->bbox3D.maxs[VZ] : 0.0f,
                    world->bspTree.numPolys, static_cast<int>(world->bspTree.bspTreeMode)));
    const auto describe = [](const char *what, zCPolygon *poly) {
        if (!poly)
        {
            Out(std::format("navgeo {}: none", what));
            return;
        }
        std::string text = std::format("navgeo {}: {} verts, normal {:.2f} {:.2f} {:.2f}, material {}:", what,
                                       static_cast<int>(poly->polyNumVert), poly->polyPlane.normal[VX],
                                       poly->polyPlane.normal[VY], poly->polyPlane.normal[VZ],
                                       poly->material ? poly->material->GetName().ToChar() : "-");
        for (int k = 0; k < poly->polyNumVert && k < 6; ++k)
            text += std::format(" ({:.0f} {:.0f} {:.0f})", poly->vertex[k]->position[VX],
                                poly->vertex[k]->position[VY], poly->vertex[k]->position[VZ]);
        Out(text);
    };
    const int flags = zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE | zTRACERAY_POLY_NORMAL;
    if (world->TraceRayNearestHit(hero + zVEC3(0.0f, 100.0f, 0.0f), zVEC3(0.0f, -1000.0f, 0.0f), player, flags))
    {
        zCPolygon *under = world->traceRayReport.foundPoly;
        describe("under the hero", under);
        if (mesh && mesh->polyArray && under)
        {
            const long long offset = reinterpret_cast<char *>(under) - reinterpret_cast<char *>(mesh->polyArray);
            Out(std::format("navgeo: the polygon is {} bytes into polyArray (element size {}, remainder {})", offset,
                            sizeof(zCPolygon), offset % static_cast<long long>(sizeof(zCPolygon))));
        }
    }
    const std::vector<zCPolygon *> polys = NavMesh::Collector::StaticPolygons(world);
    for (size_t i = 0; i < polys.size() && i < 3; ++i)
        describe("static poly", polys[i]);
    Out(std::format("navgeo: {} static polygons", polys.size()));
}

// What the engine finds below a point: the height and the polygon or object hit.
std::string DescribeGroundAt(const zVEC3 &point)
{
    oCWorld *world = ogame->GetGameWorld();
    const int flags = zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER |
                      zTRACERAY_POLY_NORMAL;
    if (!world->TraceRayNearestHit(point + zVEC3(0.0f, 300.0f, 0.0f), zVEC3(0.0f, -1500.0f, 0.0f), player, flags))
        return "nothing";
    const zTTraceRayReport &report = world->traceRayReport;
    std::string what;
    if (report.foundVob)
    {
        zCVisual *visual = report.foundVob->GetVisual();
        what = std::format("object {} {}", report.foundVob->GetObjectName().ToChar(),
                           visual ? visual->GetVisualName().ToChar() : "");
    }
    else if (report.foundPoly && report.foundPoly->material)
        what = std::format("polygon {}", report.foundPoly->material->GetName().ToChar());
    return std::format("y {:.0f} normal {:.2f} {}", report.foundIntersection[VY], report.foundPolyNormal[VY], what);
}

// navmesh probe x1 z1 x2 z2: walks the mesh surface from one point toward the other and says where and why it ends.
void CommandNavProbe(std::istringstream &in)
{
    float x1 = 0, z1 = 0, x2 = 0, z2 = 0;
    if (!(in >> x1 >> z1 >> x2 >> z2) || !NavMesh::Ready())
    {
        Out("navmesh probe: usage navmesh probe x1 z1 x2 z2 (mesh must be ready)");
        return;
    }
    // Both ends at the hero's height: stand near the place to probe.
    const float heroY = player->GetPositionWorld()[VY];
    zVEC3 a(x1, heroY, z1);
    zVEC3 b(x2, heroY, z2);
    zVEC3 snapA;
    const int start = NavMesh::NodeAt(a, &snapA, 200.0f, 800.0f);
    zVEC3 snapB;
    const int end = NavMesh::NodeAt(b, &snapB, 200.0f, 800.0f);
    Out(std::format("navmesh probe: start {:.0f} {:.0f} {:.0f} polygon {} island {}; end {:.0f} {:.0f} {:.0f} polygon {} "
                    "island {}",
                    a[VX], a[VY], a[VZ], start, NavMesh::IslandOf(start), b[VX], b[VY], b[VZ], end,
                    NavMesh::IslandOf(end)));
    if (start < 0)
        return;

    const std::vector<int> along = NavCore::NodesAlong(*NavMesh::Current->query, NavMesh::Current->graph, start,
                                                       snapA.n, snapB.n, 100000.0f);
    const int last = along.empty() ? start : along.back();
    const float *c = NavMesh::Current->graph.Center(last);
    Out(std::format("navmesh probe: the surface walk crosses {} polygons, the last one centred at {:.0f} {:.0f} {:.0f}",
                    along.size(), c[0], c[1], c[2]));

    // The mesh along the straight line every 10 cm: where the polygon changes or the mesh has a gap, what the engine
    // finds below, and the lowest thing above the ground within 2.5 m.
    oCWorld *world = ogame->GetGameWorld();
    const float length = DistanceXZ(a, b);
    const int steps = (std::min)(2000, static_cast<int>(length / 10.0f));
    int previous = -2;
    for (int i = 0; i <= steps; ++i)
    {
        const float t = steps ? static_cast<float>(i) / steps : 0.0f;
        const zVEC3 p(a[VX] + (b[VX] - a[VX]) * t, heroY + 100.0f, a[VZ] + (b[VZ] - a[VZ]) * t);
        zVEC3 snapped;
        const int node = NavMesh::NodeAt(p, &snapped, 5.0f, 600.0f);
        if (node == previous)
            continue;
        previous = node;
        std::string ceiling = "none";
        const int flags = zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER;
        if (world->TraceRayNearestHit(p + zVEC3(0.0f, 300.0f, 0.0f), zVEC3(0.0f, -1500.0f, 0.0f), player, flags))
        {
            const float groundY = world->traceRayReport.foundIntersection[VY];
            if (world->TraceRayNearestHit(zVEC3(p[VX], groundY + 10.0f, p[VZ]), zVEC3(0.0f, 250.0f, 0.0f), player,
                                          flags))
            {
                const zTTraceRayReport &report = world->traceRayReport;
                ceiling = std::format("{:.0f} cm up: {}", report.foundIntersection[VY] - groundY,
                                      report.foundVob ? report.foundVob->GetVisual()
                                                            ? report.foundVob->GetVisual()->GetVisualName().ToChar()
                                                            : "object"
                                      : report.foundPoly && report.foundPoly->material
                                          ? report.foundPoly->material->GetName().ToChar()
                                          : "?");
            }
        }
        int links = 0, tileX = -1, tileY = -1, edges = -1;
        NavMesh::Current->graph.Describe(node, links, tileX, tileY, edges);
        std::string neighbours;
        for (const int n : NavMesh::Current->graph.Neighbours(node))
            neighbours += std::format("{} ", n);
        Out(std::format("navmesh probe {:.0f} {:.0f}: mesh {} island {} area {} tile {} {} edges {} links {} [{}] at "
                        "{:.0f} | {} | ceiling {}",
                        p[VX], p[VZ], node, NavMesh::IslandOf(node),
                        node >= 0 ? NavMesh::Current->graph.AreaOf(node) : -1, tileX, tileY, edges, links,
                        neighbours, snapped[VY], DescribeGroundAt(p), ceiling));
    }
}

// navmesh [info] | rebuild | export [file] [input] | probe x1 z1 x2 z2 | geometry
void CommandNavMesh(std::istringstream &in)
{
    std::string what;
    in >> what;
    if (what == "geometry")
    {
        CommandNavGeometry();
        return;
    }
    if (what == "probe")
    {
        CommandNavProbe(in);
        return;
    }
    if (what == "gates" && NavMesh::Ready())
    {
        // Every gate: its mover state, where it is now and where it moves, and how many of its polygons it blocks now.
        const NavMesh::State &state = *NavMesh::Current;
        for (size_t g = 0; g < state.gates.size(); ++g)
        {
            const NavMesh::GateInfo &gate = state.gates[g];
            int nodes = 0, down = 0;
            for (const auto &[node, index] : state.gateNodes)
                if (index == static_cast<int>(g))
                {
                    ++nodes;
                    down += NavMesh::GateDown(gate, node) ? 1 : 0;
                }
            zCMover *mover = static_cast<zCMover *>(gate.vob);
            Out(std::format("navmesh gate {} state {} now y {:.0f}..{:.0f}, moves within y {:.0f}..{:.0f}, at {:.0f} "
                            "{:.0f}: {} polygons, {} blocked",
                            gate.vob->GetObjectName().ToChar(), static_cast<int>(mover->moverState),
                            gate.vob->bbox3D.mins[VY], gate.vob->bbox3D.maxs[VY], gate.range.mins[VY],
                            gate.range.maxs[VY], gate.vob->GetPositionWorld()[VX], gate.vob->GetPositionWorld()[VZ],
                            nodes, down));
        }
        return;
    }
    if (what == "polys")
    {
        // navmesh polys x0 z0 x1 z1 y0 y1: static polygons in a box, with their flags and material.
        float x0 = 0, z0 = 0, x1 = 0, z1 = 0, y0 = 0, y1 = 0;
        in >> x0 >> z0 >> x1 >> z1 >> y0 >> y1;
        int shown = 0;
        for (zCPolygon *poly : NavMesh::Collector::StaticPolygons(ogame->GetGameWorld()))
        {
            float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
            for (int k = 0; k < poly->polyNumVert; ++k)
                for (int a = 0; a < 3; ++a)
                {
                    lo[a] = (std::min)(lo[a], poly->vertex[k]->position[a]);
                    hi[a] = (std::max)(hi[a], poly->vertex[k]->position[a]);
                }
            if (hi[0] < x0 || lo[0] > x1 || hi[2] < z0 || lo[2] > z1 || hi[1] < y0 || lo[1] > y1)
                continue;
            zCMaterial *m = poly->material;
            Out(std::format("navmesh poly: y {:.0f}..{:.0f} normal {:.2f} portal {} occluder {} sector {} lod {} "
                            "ghost {} indoorOutdoor {} material {} group {} noCollDet {}",
                            lo[1], hi[1], poly->polyPlane.normal[VY], static_cast<int>(poly->flags.portalPoly),
                            static_cast<int>(poly->flags.occluder), static_cast<int>(poly->flags.sectorPoly),
                            static_cast<int>(poly->flags.lodFlag), static_cast<int>(poly->flags.ghostOccluder),
                            static_cast<int>(poly->flags.portalIndoorOutdoor), m ? m->GetName().ToChar() : "-",
                            m ? static_cast<int>(m->matGroup) : -1, m ? static_cast<int>(m->noCollDet) : -1));
            if (++shown >= 40)
                break;
        }
        return;
    }
    if (what == "rebuild")
    {
        NavMesh::Rebuild();
        Out("navmesh: cache removed, rebuilding");
        return;
    }
    if (what == "export")
    {
        std::string file = "yaga_navmesh.obj";
        std::string withInput;
        in >> file >> withInput;
        const bool ok = NavMesh::Export(ToWString(WaynetPatch::GameFolder() + "\\" + file), withInput == "input");
        Out(std::format("navmesh export {}: {}", file, ok ? "written" : "failed"));
        return;
    }

    if (!NavMesh::Ready())
    {
        Out(std::format("navmesh: not ready{}", NavMesh::Building() ? ", still building" : ""));
        return;
    }
    const NavCore::Graph &graph = NavMesh::Current->graph;
    const zVEC3 hero = player->GetPositionWorld();
    zVEC3 snapped;
    const int node = NavMesh::NodeAt(hero, &snapped);
    const int island = graph.Island(node);
    Out(std::format("navmesh {}: {} polygons, {} islands, {} ladder links; hero on polygon {} area {} island {} ({} "
                    "polygons), {:.0f} cm from the mesh",
                    NavMesh::Current->world, graph.Count(), graph.IslandCount(), graph.Links().size() / 2, node,
                    node >= 0 ? graph.AreaOf(node) : -1, island, graph.IslandSize(island),
                    node >= 0 ? hero.Distance(snapped) : -1.0f));
}

// Graph check without walking: one search from the hero, then which sectors (and way net components) it reaches.
void CommandRouteAll()
{
    if (NavMesh::Ready())
        CommandRouteAllMesh();

    zCWayNet *wayNet = Navigator::GetWayNet();
    const zVEC3 hero = player->GetPositionWorld();
    Navigator::Blocked blocked;
    zCWaypoint *start = Navigator::ChooseStartWaypoint(wayNet, hero, blocked);
    if (!start)
    {
        Out("routeall: no start waypoint");
        return;
    }

    std::unordered_map<zCWaypoint *, float> distance;
    std::unordered_map<zCWaypoint *, zCWaypoint *> parent;
    Navigator::ShortestPaths(start, blocked, distance, parent);

    std::map<std::pair<LONG, LONG>, std::pair<int, int>> sectors; // waypoints, reached
    Navigator::ForEachWaypoint(wayNet, [&](zCWaypoint *wp) {
        const SECTOR sector = SectorOf(wp->GetPositionWorld());
        auto &entry = sectors[{sector.x, sector.y}];
        ++entry.first;
        if (distance.count(wp))
            ++entry.second;
    });

    std::string unreachable;
    int reached = 0;
    for (const auto &[key, counts] : sectors)
    {
        if (counts.second)
            ++reached;
        else
            unreachable += std::format("({},{}):{} ", key.first, key.second, counts.first);
    }

    Out(std::format("routeall from {}: {} sectors with waypoints, {} reachable, {} waypoints reachable",
                    Navigator::WaypointName(start), sectors.size(), reached, distance.size()));
    Out("routeall unreachable sectors: " + unreachable);

    // Cut-off components: size, a sample waypoint and where they are.
    std::unordered_map<zCWaypoint *, int> component;
    std::vector<int> sizes;
    Components(wayNet, component, sizes);
    std::map<int, std::vector<zCWaypoint *>> members;
    for (const auto &[wp, id] : component)
        if (!distance.count(wp))
            members[id].push_back(wp);
    for (auto &[id, list] : members)
    {
        std::sort(list.begin(), list.end(), [](zCWaypoint *a, zCWaypoint *b) {
            return Navigator::WaypointName(a) < Navigator::WaypointName(b);
        });
        zVEC3 centre(0.0f, 0.0f, 0.0f);
        for (zCWaypoint *wp : list)
            centre += wp->GetPositionWorld();
        centre /= static_cast<float>(list.size());
        const SECTOR sector = SectorOf(centre);
        Out(std::format("routeall cut off: component {} with {} waypoints around {:.0f} {:.0f} {:.0f} (sector {} {}), "
                        "e.g. {}",
                        id, list.size(), centre[VX], centre[VY], centre[VZ], sector.x, sector.y,
                        Navigator::WaypointName(list.front())));
    }
}

// Times the engine's A* between random waypoint pairs, to see what route planning actually costs.
void CommandBench(const int count)
{
    zCWayNet *wayNet = Navigator::GetWayNet();
    std::vector<zCWaypoint *> all;
    for (auto node = wayNet->wplist.next; node; node = node->next)
        if (node->data)
            all.push_back(node->data);
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
    oCMobInter *mob = player->GetInteractMob();
    Out(std::format("diag walkmode {} wmode {} inWater {} standing {} bodystate {} interact {} (state {}/{}) em [{}] "
                    "nav {} phase {} index {}/{}",
                    ani ? ani->walkmode : -1, ani ? ani->wmode : -1, ani ? ani->IsInWater() : -1,
                    ani ? ani->IsStanding() : -1, player->GetBodyState(),
                    mob ? mob->GetObjectName().ToChar() : "none", mob ? mob->state : -1, mob ? mob->state_num : -1,
                    messages, Navigator::Active, static_cast<int>(Navigator::CurrentPhase), Navigator::Index,
                    Navigator::Path.size()));
}

// ---------------------------------------------------------------- teleporting

// Ground below a point: the highest walkable surface under x/z, searched from just under "from".
bool GroundBelow(const float x, const float fromY, const float z, float &ground)
{
    oCWorld *world = ogame->GetGameWorld();
    const zVEC3 origin(x, fromY, z);
    const zVEC3 ray(0.0f, -20000.0f, 0.0f);
    const int flags = zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER |
                      zTRACERAY_POLY_IGNORE_TRANSP;
    if (!world->TraceRayNearestHit(origin, ray, player, flags))
        return false;
    ground = world->traceRayReport.foundIntersection[VY];
    return true;
}

void BeamToPosition(const zVEC3 &position)
{
    Navigator::Stop(Navigator::StopReason::Cancelled);
    player->SetCollDet(FALSE);
    player->SetPositionWorld(position);
    player->SetCollDet(TRUE);
    if (oCSpawnManager *spawn = ogame->GetSpawnManager())
        spawn->SpawnImmediately(TRUE);
}

// A way net waypoint, a free point or vob name (the engine's BeamTo), or a patch node.
bool Beam(const std::string &name)
{
    Navigator::Stop(Navigator::StopReason::Cancelled);
    const std::string upper = WaynetPatch::Upper(name);
    Navigator::GetWayNet(); // builds the patch
    const auto node = WaynetPatch::ByName.find(upper);
    if (node != WaynetPatch::ByName.end())
    {
        BeamToPosition(node->second->GetPositionWorld() + zVEC3(0.0f, 50.0f, 0.0f));
        return true;
    }
    return player->BeamTo(zSTRING(upper.c_str())) != 0;
}

// ---------------------------------------------------------------- test mode

inline bool Peace = false;
constexpr float PeaceRadius = 5000.0f;

// Removes monsters and orcs near the hero and calms hostile humans, so a low-level hero can walk anywhere. Humans are
// never deleted: scripts keep plain pointers to them.
void PeaceSweep()
{
    oCWorld *world = ogame->GetGameWorld();
    if (!world || !player)
        return;

    const zVEC3 hero = player->GetPositionWorld();
    std::vector<oCNpc *> doomed;
    for (zCListSort<oCNpc> *node = world->voblist_npcs; node; node = node->next)
    {
        oCNpc *npc = node->data;
        if (!npc || npc == player || npc->IsDead())
            continue;
        if (hero.Distance(npc->GetPositionWorld()) > PeaceRadius)
            continue;

        if (npc->IsMonster())
            doomed.push_back(npc);
        else if (npc->enemy == player || npc->GetAttitude(player) == NPC_ATT_HOSTILE)
        {
            npc->SetTmpAttitude(NPC_ATT_FRIENDLY);
            npc->SetEnemy(nullptr);
            Out(std::format("peace: calmed {}", npc->GetName(0).ToChar()));
        }
    }

    if (doomed.empty())
        return;

    // Sound cues and the tracker keep plain vob pointers.
    SpatialCues::StopAll();
    VobTracker::Candidates.clear();
    VobTracker::SelectedVob = nullptr;

    oCSpawnManager *spawn = ogame->GetSpawnManager();
    for (oCNpc *npc : doomed)
    {
        Out(std::format("peace: removed {} ({}) at {:.0f} cm", npc->GetName(0).ToChar(),
                        npc->GetInstanceName().ToChar(), hero.Distance(npc->GetPositionWorld())));
        spawn->DeleteNpc(npc);
    }
}

void SetPeace(const bool on)
{
    Peace = on;
    if (oCSpawnManager *spawn = ogame->GetSpawnManager())
        spawn->SetSpawningEnabled(on ? FALSE : TRUE);
    Out(std::format("peace {}", on ? "on" : "off"));
    if (on)
        PeaceSweep();
}

// ---------------------------------------------------------------- dumps

std::string Name(const zSTRING &text)
{
    std::string name = text.ToChar();
    for (char &c : name)
        if (c == ' ' || c == '\t')
            c = '_';
    return name.empty() ? "-" : name;
}

template <typename Fn> void ForEachVobOfClass(zCClassDef *classDef, Fn fn)
{
    zCArray<zCVob *> vobs;
    ogame->GetGameWorld()->SearchVobListByBaseClass(classDef, vobs, nullptr);
    for (int i = 0; i < vobs.GetNumInList(); ++i)
        if (vobs[i])
            fn(vobs[i]);
}

// Everything that helps to find places and gaps: waypoints with their components, ways, music zones (named areas),
// level changes, gates, ladders, doors, free points, mobs and NPCs.
void CommandDumpNet(const std::string &fileName)
{
    oCWorld *world = ogame->GetGameWorld();
    zCWayNet *wayNet = Navigator::GetWayNet();
    std::ofstream out(fileName, std::ios::binary);
    if (!out)
    {
        Out("dumpnet: cannot write " + fileName);
        return;
    }

    const zTBBox3D box = world->GetBspTree()->bspRoot->bbox3D;
    out << std::format("world {} bbox {:.0f} {:.0f} {:.0f} {:.0f} {:.0f} {:.0f}\n", WaynetPatch::WorldFileName(world),
                       box.mins[VX], box.mins[VY], box.mins[VZ], box.maxs[VX], box.maxs[VY], box.maxs[VZ]);

    std::unordered_map<zCWaypoint *, int> component;
    std::vector<int> sizes;
    Components(wayNet, component, sizes);

    int counts[10] = {};
    Navigator::ForEachWaypoint(wayNet, [&](zCWaypoint *wp) {
        const zVEC3 p = wp->GetPositionWorld();
        int degree = 0;
        Navigator::ForEachNeighbour(wp, [&](zCWaypoint *) { ++degree; });
        out << std::format("wp {} {:.0f} {:.0f} {:.0f} comp {} size {} deg {} patch {}\n", Navigator::WaypointName(wp),
                           p[VX], p[VY], p[VZ], component[wp], sizes[component[wp]], degree,
                           WaynetPatch::IsOurs(wp) ? 1 : 0);
        ++counts[0];
    });

    for (auto node = wayNet->waylist.next; node; node = node->next)
    {
        zCWay *way = node->data;
        if (!way || !way->left || !way->right)
            continue;
        oCWay *oway = static_cast<oCWay *>(way); // the game's object factory makes every way an oCWay
        out << std::format("way {} {} ladder {} door {} avoid {}\n", Navigator::WaypointName(way->left),
                           Navigator::WaypointName(way->right), oway->ladder ? Name(oway->ladder->GetObjectName()) : "-",
                           oway->door ? Name(oway->door->GetObjectName()) : "-",
                           WaynetPatch::IsAvoided(way->left, way->right) ? 1 : 0);
        ++counts[1];
    }
    for (const auto &[wp, list] : WaynetPatch::Links)
        for (zCWaypoint *next : list)
            if (wp < next)
                out << std::format("link {} {} jump {}\n", Navigator::WaypointName(wp), Navigator::WaypointName(next),
                                   WaynetPatch::IsJumpLeg(wp, next) ? 1 : 0);

    ForEachVobOfClass(oCZoneMusic::classDef, [&](zCVob *vob) {
        if (zDYNAMIC_CAST<oCZoneMusicDefault>(vob))
            return;
        const zTBBox3D &b = vob->bbox3D;
        out << std::format("zone {} {:.0f} {:.0f} {:.0f} {:.0f} {:.0f} {:.0f}\n", Name(vob->GetObjectName()),
                           b.mins[VX], b.mins[VY], b.mins[VZ], b.maxs[VX], b.maxs[VY], b.maxs[VZ]);
        ++counts[2];
    });

    ForEachVobOfClass(oCTriggerChangeLevel::classDef, [&](zCVob *vob) {
        oCTriggerChangeLevel *trigger = static_cast<oCTriggerChangeLevel *>(vob);
        const zVEC3 p = vob->GetPositionWorld();
        out << std::format("level {} {:.0f} {:.0f} {:.0f} to {} {}\n", Name(vob->GetObjectName()), p[VX], p[VY], p[VZ],
                           Name(trigger->levelName), Name(trigger->startVob));
        ++counts[3];
    });

    ForEachVobOfClass(zCMover::classDef, [&](zCVob *vob) {
        zCMover *mover = static_cast<zCMover *>(vob);
        const zVEC3 p = vob->GetPositionWorld();
        out << std::format("mover {} {:.0f} {:.0f} {:.0f} state {} visual {}\n", Name(vob->GetObjectName()), p[VX],
                           p[VY], p[VZ], static_cast<int>(mover->moverState),
                           vob->GetVisual() ? Name(vob->GetVisual()->GetVisualName()) : "-");
        ++counts[4];
    });

    ForEachVobOfClass(zCVobSpot::classDef, [&](zCVob *vob) {
        const zVEC3 p = vob->GetPositionWorld();
        out << std::format("fp {} {:.0f} {:.0f} {:.0f}\n", Name(vob->GetObjectName()), p[VX], p[VY], p[VZ]);
        ++counts[5];
    });

    for (zCListSort<zCVob> *node = world->voblist; node; node = node->next)
    {
        zCVob *vob = node->data;
        oCMOB *mob = zDYNAMIC_CAST<oCMOB>(vob);
        if (!mob)
            continue;
        const zVEC3 p = vob->GetPositionWorld();
        const char *kind = zDYNAMIC_CAST<oCMobLadder>(vob) ? "ladder"
                           : zDYNAMIC_CAST<oCMobDoor>(vob) ? (Navigator::IsDoor(static_cast<oCMobDoor *>(vob)) ? "door" : "doorlike")
                           : zDYNAMIC_CAST<oCMobContainer>(vob) ? "chest"
                           : zDYNAMIC_CAST<oCMobInter>(vob) ? "inter"
                                                             : "mob";
        oCMobDoor *door = zDYNAMIC_CAST<oCMobDoor>(vob);
        out << std::format("mob {} {} {} {:.0f} {:.0f} {:.0f} locked {}\n", kind, Name(vob->GetObjectName()),
                           Name(mob->GetName()), p[VX], p[VY], p[VZ], door ? static_cast<int>(door->locked) : 0);
        ++counts[6];
    }

    for (zCListSort<oCNpc> *node = world->voblist_npcs; node; node = node->next)
    {
        oCNpc *npc = node->data;
        if (!npc)
            continue;
        const zVEC3 p = npc->GetPositionWorld();
        out << std::format("npc {} {} guild {} monster {} {:.0f} {:.0f} {:.0f}\n", Name(npc->GetInstanceName()),
                           Name(npc->GetName(0)), npc->guild, npc->IsMonster() ? 1 : 0, p[VX], p[VY], p[VZ]);
        ++counts[7];
    }

    Out(std::format("dumpnet {}: {} waypoints, {} ways, {} zones, {} level changes, {} movers, {} free points, "
                    "{} mobs, {} npcs",
                    fileName, counts[0], counts[1], counts[2], counts[3], counts[4], counts[5], counts[6], counts[7]));
}

// Ground heights on a grid, every layer from the sky down (a bridge over a river gives two), with water surfaces and
// the steepness of each hit polygon. Runs a slice per frame.
namespace Heightmap
{
inline bool Active = false;
inline std::ofstream File;
inline float X1 = 0.0f, Z1 = 0.0f, Step = 200.0f;
inline int Nx = 0, Nz = 0, Next = 0;
inline float Top = 0.0f, Bottom = 0.0f;
inline std::string FileName;

void Start(const float x1, const float z1, const float x2, const float z2, const float step, const std::string &name)
{
    oCWorld *world = ogame->GetGameWorld();
    const zTBBox3D box = world->GetBspTree()->bspRoot->bbox3D;
    X1 = (std::min)(x1, x2);
    Z1 = (std::min)(z1, z2);
    Step = (std::max)(25.0f, step);
    Nx = static_cast<int>(std::abs(x2 - x1) / Step) + 1;
    Nz = static_cast<int>(std::abs(z2 - z1) / Step) + 1;
    Top = box.maxs[VY] + 100.0f;
    Bottom = box.mins[VY] - 100.0f;
    Next = 0;
    FileName = name;
    File.open(name, std::ios::binary | std::ios::trunc);
    File << std::format("heightmap {:.0f} {:.0f} {:.0f} {} {}\n", X1, Z1, Step, Nx, Nz);
    Active = true;
    Out(std::format("heightmap {} cells into {}", Nx * Nz, name));
}

void Update()
{
    if (!Active)
        return;

    oCWorld *world = ogame->GetGameWorld();
    const int flags = zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER |
                      zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_POLY_NORMAL | zTRACERAY_POLY_TEST_WATER;
    const auto started = std::chrono::high_resolution_clock::now();
    const int total = Nx * Nz;

    std::string chunk;
    while (Next < total)
    {
        const int i = Next % Nx;
        const int j = Next / Nx;
        const float x = X1 + i * Step;
        const float z = Z1 + j * Step;
        chunk += std::format("{} {}", i, j);

        float y = Top;
        for (int layer = 0; layer < 6 && y > Bottom; ++layer)
        {
            if (!world->TraceRayNearestHit(zVEC3(x, y, z), zVEC3(0.0f, Bottom - y, 0.0f), player, flags))
                break;

            const zTTraceRayReport &report = world->traceRayReport;
            const float hitY = report.foundIntersection[VY];
            zCPolygon *poly = report.foundPoly;
            const bool water = poly && poly->material && poly->material->matGroup == zMAT_GROUP_WATER;
            const float normalY = poly ? report.foundPolyNormal[VY] : 1.0f;
            const char kind = water ? 'w' : (report.foundVob ? 'v' : 's');
            chunk += std::format(" {:.0f}{}{:.0f}", hitY, kind, normalY * 100.0f);
            y = hitY - 40.0f;
        }
        chunk += "\n";
        ++Next;

        if ((Next & 63) == 0 &&
            std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - started).count() > 25.0)
            break;
    }
    File << chunk;

    if (Next >= total)
    {
        File.close();
        Active = false;
        Out(std::format("heightmap done: {}", FileName));
    }
}
} // namespace Heightmap

// ---------------------------------------------------------------- driving and recording

// Runs the hero straight at a point with the engine's animation controller (jumping when stuck); for exploring ground
// without waypoints.
namespace Drive
{
inline bool Active = false;
inline zVEC3 Target;
inline zVEC3 Reference;
inline float ReferenceAt = 0.0f;
inline bool Jumped = false;

void Start(const float x, const float z)
{
    Navigator::Stop(Navigator::StopReason::Cancelled);
    Target = zVEC3(x, 0.0f, z);
    Reference = player->GetPositionWorld();
    ReferenceAt = GameMs();
    Jumped = false;
    Active = true;
    Out(std::format("drive to {:.0f} {:.0f}, {:.0f} cm", x, z, DistanceXZ(Reference, Target)));
}

void Finish(const char *how)
{
    Active = false;
    player->anictrl->_Stand();
    const zVEC3 p = player->GetPositionWorld();
    Out(std::format("drive {} at {:.0f} {:.0f} {:.0f}, {:.0f} cm from target", how, p[VX], p[VY], p[VZ],
                    DistanceXZ(p, Target)));
}

void Update()
{
    if (!Active)
        return;
    if (Navigator::PlayerWantsControl())
    {
        Finish("cancelled");
        return;
    }

    const zVEC3 hero = player->GetPositionWorld();
    if (DistanceXZ(hero, Target) < 150.0f)
    {
        Finish("arrived");
        return;
    }

    zVEC3 look = Target;
    look[VY] = hero[VY];
    player->SetHeadingYWorld(look);
    player->anictrl->_Forward();

    if (DistanceXZ(hero, Reference) > Navigator::StuckMinMove)
    {
        Reference = hero;
        ReferenceAt = GameMs();
        Jumped = false;
        return;
    }

    const float idle = GameMs() - ReferenceAt;
    if (!Jumped && idle > Navigator::DriveJumpAfterMs)
    {
        Jumped = true;
        if (!player->anictrl->IsInWater())
            player->anictrl->PC_JumpForward();
    }
    else if (idle > Navigator::DriveGiveUpMs)
        Finish("stuck");
}
} // namespace Drive

// Records the hero's walk as patch nodes (one every ~8 m or at a height change) into yaga_rec.txt.
namespace Recorder
{
inline bool Active = false;
inline std::string Prefix;
inline std::vector<zVEC3> Points;

void Start(const std::string &prefix)
{
    Prefix = WaynetPatch::Upper(prefix);
    Points = {player->GetPositionWorld()};
    Active = true;
    Out("rec start " + Prefix);
}

void Update()
{
    if (!Active)
        return;
    const zVEC3 hero = player->GetPositionWorld();
    const zVEC3 &last = Points.back();
    if (DistanceXZ(hero, last) > 800.0f || (DistanceXZ(hero, last) > 300.0f && std::abs(hero[VY] - last[VY]) > 150.0f))
        Points.push_back(hero);
}

void Stop()
{
    if (!Active)
        return;
    Active = false;
    Points.push_back(player->GetPositionWorld());

    zCWayNet *wayNet = Navigator::GetWayNet();
    std::ofstream out("yaga_rec.txt", std::ios::app);
    out << std::format("# recorded {}: {} points, start near {}, end near {}\n", Prefix, Points.size(),
                       Navigator::WaypointName(wayNet->GetNearestWaypoint(Points.front())),
                       Navigator::WaypointName(wayNet->GetNearestWaypoint(Points.back())));
    for (size_t i = 0; i < Points.size(); ++i)
        out << std::format("node {}_{:02} {:.0f} {:.0f} {:.0f}\n", Prefix, i, Points[i][VX], Points[i][VY] - 90.0f,
                           Points[i][VZ]);
    for (size_t i = 1; i < Points.size(); ++i)
        out << std::format("link {}_{:02} {}_{:02}\n", Prefix, i - 1, Prefix, i);
    Out(std::format("rec stop {}: {} points written to yaga_rec.txt", Prefix, Points.size()));
}
} // namespace Recorder

// ---------------------------------------------------------------- tour

// Walks a list of destinations one after another and logs how each ended. File lines:
//   LABEL wp NAME [@START]
//   LABEL sector X Y [@START]
//   LABEL pos X Z [@START]
// START is a waypoint, free point, vob or patch node name, or X,Z, to teleport to before the walk.
namespace Tour
{
struct Item
{
    std::string Label;
    std::string Kind;
    std::string Name;
    float A = 0.0f;
    float B = 0.0f;
    std::string Start;
};

enum class Stage
{
    Idle,
    Settling,
    Walking
};

inline std::vector<Item> Items;
inline size_t Next = 0;
inline bool Running = false;
inline Stage CurrentStage = Stage::Idle;
inline float StageStartedAt = 0.0f;
inline int OutcomesBefore = 0;
inline float TimeoutMs = 600000.0f;
inline int Arrived = 0;
inline int Failed = 0;
inline std::vector<std::string> Failures;

bool Load(const std::string &fileName)
{
    std::ifstream in(fileName);
    if (!in)
    {
        Out("tour: cannot read " + fileName);
        return false;
    }

    Items.clear();
    for (std::string line; std::getline(in, line);)
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        const size_t comment = line.find('#');
        if (comment != std::string::npos)
            line.resize(comment);

        std::istringstream words(line);
        Item item;
        if (!(words >> item.Label >> item.Kind))
            continue;

        bool ok = true;
        if (item.Kind == "wp")
            ok = static_cast<bool>(words >> item.Name);
        else if (item.Kind == "sector" || item.Kind == "pos")
            ok = static_cast<bool>(words >> item.A >> item.B);
        else
            ok = false;

        std::string start;
        if (words >> start && start.size() > 1 && start[0] == '@')
            item.Start = start.substr(1);

        if (ok)
            Items.push_back(item);
        else
            Out("tour: bad line: " + line);
    }
    return !Items.empty();
}

void StartTour(const std::string &fileName)
{
    if (!Load(fileName))
        return;
    Next = 0;
    Arrived = 0;
    Failed = 0;
    Failures.clear();
    CurrentStage = Stage::Idle;
    Running = true;
    Out(std::format("tour: {} destinations from {}", Items.size(), fileName));
}

void StopTour()
{
    if (!Running)
        return;
    Running = false;
    Navigator::Stop(Navigator::StopReason::Cancelled);
    Out("tour stopped");
}

bool BeamStart(const std::string &start)
{
    const size_t comma = start.find(',');
    if (comma != std::string::npos)
    {
        const float x = std::stof(start.substr(0, comma));
        const float z = std::stof(start.substr(comma + 1));
        float ground = 0.0f;
        const zTBBox3D box = ogame->GetGameWorld()->GetBspTree()->bspRoot->bbox3D;
        if (!GroundBelow(x, box.maxs[VY] + 100.0f, z, ground))
            return false;
        BeamToPosition(zVEC3(x, ground + 100.0f, z));
        return true;
    }
    return Beam(start);
}

const char *ReasonName(const Navigator::StopReason reason)
{
    switch (reason)
    {
    case Navigator::StopReason::Arrived:
        return "Arrived";
    case Navigator::StopReason::Cancelled:
        return "Cancelled";
    case Navigator::StopReason::Attacked:
        return "Attacked";
    case Navigator::StopReason::NoRoute:
        return "NoRoute";
    case Navigator::StopReason::GaveUp:
        return "GaveUp";
    case Navigator::StopReason::WorldChanged:
        return "WorldChanged";
    case Navigator::StopReason::Dead:
        return "Dead";
    case Navigator::StopReason::TargetLost:
        return "TargetLost";
    }
    return "?";
}

void Record(const Item &item, const bool timedOut)
{
    const Navigator::Outcome &outcome = Navigator::LastOutcome;
    const bool arrived = outcome.Reason == Navigator::StopReason::Arrived && !timedOut;
    if (arrived)
        ++Arrived;
    else
    {
        ++Failed;
        Failures.push_back(item.Label);
    }

    const zVEC3 p = player->GetPositionWorld();
    const SECTOR sector = SectorOf(p);
    YagaLog::Write(std::format("[tour] {} {}: {}{} {:.1f} s, {} recalculations, {} blocked legs, {} m left, hero {:.0f} "
                               "{:.0f} {:.0f} sector {} {}",
                               arrived ? "OK" : "FAIL", item.Label, timedOut ? "Timeout/" : "",
                               ReasonName(outcome.Reason), outcome.Seconds, outcome.Recalculations,
                               outcome.BlockedLegs, outcome.MetersLeft, p[VX], p[VY], p[VZ], sector.x, sector.y) +
                   (outcome.ArrivedInSector ? " (goal waypoint not reached)" : ""));
}

void Update()
{
    if (!Running)
        return;

    switch (CurrentStage)
    {
    case Stage::Idle:
    {
        if (Next >= Items.size())
        {
            Running = false;
            std::string failures;
            for (const std::string &label : Failures)
                failures += label + " ";
            YagaLog::Write(std::format("[tour] done: {} arrived, {} failed: {}", Arrived, Failed, failures));
            return;
        }

        const Item &item = Items[Next];
        YagaLog::Write(std::format("[tour] {}/{} {}", Next + 1, Items.size(), item.Label));
        if (!item.Start.empty() && !BeamStart(item.Start))
            Out("tour: cannot teleport to " + item.Start);
        CurrentStage = Stage::Settling;
        StageStartedAt = GameMs();
        return;
    }

    case Stage::Settling:
    {
        if (GameMs() - StageStartedAt < 1500.0f)
            return;
        if (Peace)
            PeaceSweep();

        const Item &item = Items[Next];
        OutcomesBefore = Navigator::Outcomes;
        if (item.Kind == "wp")
            Navigator::StartToWaypoint(item.Name);
        else if (item.Kind == "sector")
        {
            SECTOR sector{};
            sector.x = static_cast<LONG>(item.A);
            sector.y = static_cast<LONG>(item.B);
            Navigator::StartToSector(sector);
        }
        else
        {
            float ground = 0.0f;
            const zTBBox3D box = ogame->GetGameWorld()->GetBspTree()->bspRoot->bbox3D;
            GroundBelow(item.A, box.maxs[VY] + 100.0f, item.B, ground);
            Navigator::StartToPosition(zVEC3(item.A, ground, item.B));
        }
        CurrentStage = Stage::Walking;
        StageStartedAt = GameMs();
        return;
    }

    case Stage::Walking:
    {
        const Item &item = Items[Next];
        bool timedOut = false;
        if (Navigator::Outcomes == OutcomesBefore)
        {
            if (GameMs() - StageStartedAt < TimeoutMs)
                return;
            timedOut = true;
            Navigator::Stop(Navigator::StopReason::GaveUp);
        }
        Record(item, timedOut);
        ++Next;
        CurrentStage = Stage::Idle;
        return;
    }
    }
}
} // namespace Tour


// Engine-only movement experiments (no keys): "efwd MS" calls _Forward every frame, "ejump" PC_JumpForward,
// "emob up|down|none" mounts the nearest mob with StartInteraction and feeds Interact every frame.
namespace EngineTest
{
inline float ForwardUntil = 0.0f;
inline oCMobInter *Mob = nullptr;
inline int MobDirection = 0; // 1 up, -1 down, 0 none
inline float MobUntil = 0.0f;
inline float NextReport = 0.0f;

oCMobInter *NearestMob()
{
    oCMobInter *best = nullptr;
    float bestDistance = 1500.0f;
    const zVEC3 hero = player->GetPositionWorld();
    for (zCListSort<zCVob> *node = ogame->GetGameWorld()->voblist; node; node = node->next)
    {
        oCMobInter *mob = zDYNAMIC_CAST<oCMobInter>(node->data);
        if (mob && hero.Distance(mob->GetPositionWorld()) < bestDistance)
        {
            best = mob;
            bestDistance = hero.Distance(mob->GetPositionWorld());
        }
    }
    return best;
}

void Update()
{
    const float now = GameMs();
    if (ForwardUntil > 0.0f)
    {
        if (now < ForwardUntil)
            player->anictrl->_Forward();
        else
        {
            ForwardUntil = 0.0f;
            player->anictrl->_Stand();
            CommandPos();
        }
    }

    if (Mob)
    {
        if (now > MobUntil)
        {
            Out("emob done");
            CommandDiag();
            CommandPos();
            Mob = nullptr;
            return;
        }
        if (player->GetInteractMob() == Mob && (MobDirection != 2 || Mob->state == 0))
            Mob->Interact(player, 1, MobDirection != 0 && MobDirection != -1, MobDirection < 0, 0, 0);
        if (now > NextReport)
        {
            NextReport = now + 1000.0f;
            Out(std::format("emob {} state {}/{} interacting {} body {} y {:.0f}", Mob->GetObjectName().ToChar(),
                            Mob->state, Mob->state_num, player->GetInteractMob() == Mob, player->GetBodyState(),
                            player->GetPositionWorld()[VY]));
        }
    }
}
} // namespace EngineTest


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
    else if (command == "routeall")
        CommandRouteAll();
    else if (command == "navmesh")
        CommandNavMesh(in);
    else if (command == "navwp")
    {
        std::string name;
        in >> name;
        Navigator::StartToWaypoint(name);
    }
    else if (command == "navpos")
    {
        float x = 0.0f, z = 0.0f;
        in >> x >> z;
        float ground = player->GetPositionWorld()[VY];
        const zTBBox3D box = ogame->GetGameWorld()->GetBspTree()->bspRoot->bbox3D;
        GroundBelow(x, box.maxs[VY] + 100.0f, z, ground);
        Navigator::StartToPosition(zVEC3(x, ground, z));
    }
    else if (command == "stop")
    {
        Navigator::Stop(Navigator::StopReason::Cancelled);
        if (Drive::Active)
            Drive::Finish("stopped");
    }
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
        if (!Beam(name))
            Out("beam: no such target " + name);
        CommandPos();
    }
    else if (command == "beampos")
    {
        std::string where;
        in >> where;
        float x = 0.0f, z = 0.0f;
        if (where.find(',') == std::string::npos)
        {
            x = std::stof(where);
            in >> z;
            where = std::format("{},{}", x, z);
        }
        if (!Tour::BeamStart(where))
            Out("beampos: no ground there");
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
        std::string text = std::format("water depth here {:.0f}", Navigator::WaterDepthAt(hero));
        for (const float metres : {1.5f, 3.0f, 6.0f, 10.0f})
            text += std::format(", {:.1f} m ahead {:.0f}", metres,
                                Navigator::WaterDepthAt(hero + at.Normalize() * (metres * 100.0f)));
        Out(text);
    }
    else if (command == "wp")
    {
        std::string name;
        in >> name;
        zCWaypoint *wp = Navigator::FindWaypoint(name);
        if (!wp)
        {
            Out("no such waypoint");
            return;
        }
        std::string ways;
        Navigator::ForEachNeighbour(wp, [&](zCWaypoint *next) { ways += Navigator::WaypointName(next) + " "; });
        const zVEC3 position = wp->GetPositionWorld();
        Out(std::format("wp {} at {:.0f} {:.0f} {:.0f} waterDepth {} underWater {} patch {} ways: {}", name,
                        position[VX], position[VY], position[VZ], wp->waterDepth, wp->underWater,
                        WaynetPatch::IsOurs(wp) ? 1 : 0, ways));
    }
    else if (command == "places")
    {
        // Every named place: sector, waypoint and whether the graph reaches it; also writes a tour that walks to each
        // place's sector the way SPACE on the map does.
        zCWayNet *wayNet = Navigator::GetWayNet();
        Navigator::Blocked blocked;
        std::unordered_map<zCWaypoint *, float> distance;
        std::unordered_map<zCWaypoint *, zCWaypoint *> parent;
        if (zCWaypoint *start = Navigator::ChooseStartWaypoint(wayNet, player->GetPositionWorld(), blocked))
            Navigator::ShortestPaths(start, blocked, distance, parent);

        std::ofstream tour("yaga_tour_places.txt");
        Places::EnsureResolved();
        for (const Places::Entry &entry : Places::Entries)
        {
            YagaLog::Write(std::format(L"[dev] place {} / {}: {} sector {} {}, {}", entry.Info->English,
                                       entry.Info->Polish, ToWString(entry.Info->Waypoint), entry.Sector.x,
                                       entry.Sector.y, distance.count(entry.Waypoint) ? L"reachable" : L"NOT reachable"));
            std::string label = WaynetPatch::Upper(entry.Info->Waypoint);
            tour << std::format("{} sector {} {}\n", label, entry.Sector.x, entry.Sector.y);
        }
        Out(std::format("{} places of {} resolved, tour written to yaga_tour_places.txt", Places::Entries.size(),
                        Places::Table().size()));
    }
    else if (command == "facemob")
    {
        // The nearest usable mob turns the hero the way the engine does before using it.
        oCMobInter *best = nullptr;
        float bestDistance = 1500.0f;
        const zVEC3 hero = player->GetPositionWorld();
        for (zCListSort<zCVob> *node = ogame->GetGameWorld()->voblist; node; node = node->next)
        {
            oCMobInter *mob = zDYNAMIC_CAST<oCMobInter>(node->data);
            if (mob && hero.Distance(mob->GetPositionWorld()) < bestDistance)
            {
                best = mob;
                bestDistance = hero.Distance(mob->GetPositionWorld());
            }
        }
        if (best)
        {
            std::string how;
            in >> how;
            if (how == "place")
                best->SetIdealPosition(player);
            best->SetHeading(player);
            Out(std::format("facemob {} at {:.0f} cm, heading {:.0f}", best->GetObjectName().ToChar(), bestDistance,
                            GetCompassAngle()));
        }
    }
    else if (command == "efwd")
    {
        float ms = 2000.0f;
        in >> ms;
        EngineTest::ForwardUntil = GameMs() + ms;
        Out(std::format("efwd {:.0f} ms", ms));
    }
    else if (command == "facewp")
    {
        std::string name;
        in >> name;
        if (zCWaypoint *wp = Navigator::FindWaypoint(name))
        {
            zVEC3 look = wp->GetPositionWorld();
            look[VY] = player->GetPositionWorld()[VY];
            player->SetHeadingYWorld(look);
            Out(std::format("facing {}", name));
        }
    }
    else if (command == "ejump")
    {
        oCAniCtrl_Human *ani = player->anictrl;
        std::string how = "pc";
        in >> how;
        Out(std::format("ejump {}: canJumpLedge {} canJump {}", how, ani->CanJumpLedge(), ani->CanJump(1)));
        if (how == "forward")
            Out(std::format("JumpForward -> {}", ani->JumpForward()));
        else
            ani->PC_JumpForward();
    }
    else if (command == "emob")
    {
        std::string how = "up";
        in >> how;
        EngineTest::Mob = EngineTest::NearestMob();
        if (!EngineTest::Mob)
        {
            Out("emob: no mob");
            return;
        }
        EngineTest::MobDirection = how == "up" ? 1 : how == "down" ? -1 : how == "open" ? 2 : 0;
        EngineTest::MobUntil = GameMs() + 15000.0f;
        EngineTest::NextReport = 0.0f;
        EngineTest::Mob->SetIdealPosition(player);
        EngineTest::Mob->SetHeading(player);
        EngineTest::Mob->StartInteraction(player);
        Out(std::format("emob start {} at {:.0f} cm", EngineTest::Mob->GetObjectName().ToChar(),
                        player->GetPositionWorld().Distance(EngineTest::Mob->GetPositionWorld())));
    }
    else if (command == "estop")
    {
        std::string how = "stop";
        in >> how;
        oCMobInter *mob = player->GetInteractMob();
        if (!mob)
            mob = EngineTest::NearestMob();
        EngineTest::Mob = nullptr;
        if (mob)
        {
            if (how == "end")
                mob->EndInteraction(player, 1);
            else if (how == "interrupt")
                mob->InterruptInteraction(player);
            else
                mob->StopInteraction(player);
        }
        CommandDiag();
    }
    else if (command == "bench")
    {
        int count = 200;
        in >> count;
        CommandBench((std::max)(1, count));
    }
    else if (command == "god")
    {
        int on = 1;
        in >> on;
        oCNpc::godmode = on ? TRUE : FALSE;
        Out(std::format("god {}", on ? "on" : "off"));
    }
    else if (command == "peace")
    {
        int on = 1;
        in >> on;
        SetPeace(on != 0);
    }
    else if (command == "speed")
    {
        float factor = 1.0f;
        in >> factor;
        ztimer->factorMotion = std::clamp(factor, 0.1f, 3.0f);
        Out(std::format("speed {:.2f}", ztimer->factorMotion));
    }
    else if (command == "dumpnet")
    {
        std::string name = "yaga_waynet_dump.txt";
        in >> name;
        CommandDumpNet(name);
    }
    else if (command == "heightmap")
    {
        float x1 = 0.0f, z1 = 0.0f, x2 = 0.0f, z2 = 0.0f, step = 400.0f;
        std::string name = "yaga_heightmap.txt";
        if (!(in >> x1 >> z1 >> x2 >> z2))
        {
            const zTBBox3D box = ogame->GetGameWorld()->GetBspTree()->bspRoot->bbox3D;
            x1 = box.mins[VX];
            z1 = box.mins[VZ];
            x2 = box.maxs[VX];
            z2 = box.maxs[VZ];
        }
        in >> step >> name;
        Heightmap::Start(x1, z1, x2, z2, step, name);
    }
    else if (command == "tour")
    {
        std::string argument = "yaga_tour.txt";
        in >> argument;
        if (argument == "stop")
            Tour::StopTour();
        else
        {
            float minutes = 0.0f;
            if (in >> minutes && minutes > 0.0f)
                Tour::TimeoutMs = minutes * 60000.0f;
            Tour::StartTour(argument);
        }
    }
    else if (command == "patch")
    {
        WaynetPatch::Reload();
        CommandWayNetInfo();
    }
    else if (command == "drive")
    {
        float x = 0.0f, z = 0.0f;
        in >> x >> z;
        Drive::Start(x, z);
    }
    else if (command == "rec")
    {
        std::string what, prefix = "REC";
        in >> what >> prefix;
        if (what == "start")
            Recorder::Start(prefix);
        else
            Recorder::Stop();
    }
    else
        Out("unknown command");
}

void Poll()
{
    if (!Settings::DevCommands || !player)
        return;

    if (Peace)
        PeaceSweep();
    Heightmap::Update();
    Drive::Update();
    Recorder::Update();
    Tour::Update();
    EngineTest::Update();

    const ULONGLONG now = GetTickCount64();
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
