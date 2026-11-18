#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <set>
#include <thread>
#include <unordered_set>

#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

#include "NavMeshCore.h"

namespace GOTHIC_NAMESPACE
{
// The navigation mesh of the world being played: every surface the hero can walk on, built by Recast from the same
// geometry the engine collides with (the static world mesh and every object with dynamic collision). It is built once
// per world in background threads and kept in <game>\YAGA\navmesh\<WORLD>.nav; later loads read that file. Until it
// is ready, or where the hero stands off it, the navigator uses the game's way net.
//
// Left out of the geometry, because the navigator handles them itself: characters and items, doors (opened on the
// way), ladders (our own links between the foot and the top) and movers (gates: their ground is marked, and blocked
// while the gate is down).
namespace NavMesh
{
constexpr float GoalExtentXZ = 300.0f; // a point counts as on the mesh within this distance sideways...
constexpr float GoalExtentY = 400.0f;  // ... and up or down
constexpr float LadderCostFactor = 1.5f;
constexpr float LadderExtraCost = 300.0f;

struct LadderInfo
{
    oCMobLadder *vob = nullptr;
    zTBBox3D box;
    zVEC3 at;
    bool linked = false;
};

struct GateInfo
{
    zCVob *vob = nullptr;
    zTBBox3D range; // everywhere the gate moves (its box at every keyframe)
};

// A build running in a background thread. The thread owns a reference, so a cancelled build can finish on its own.
struct Job
{
    NavCore::Input input;
    std::wstring cachePath;
    std::atomic<bool> cancel{false};
    std::atomic<bool> done{false};
    NavCore::NavMeshPtr mesh;
    NavCore::BuildStats stats;
    std::unique_ptr<NavCore::Graph> graph;
    bool saved = false;
};

struct State
{
    std::string world;
    uint64_t hash = 0;
    NavCore::NavMeshPtr mesh;
    NavCore::QueryPtr query;
    NavCore::Graph graph;
    std::vector<LadderInfo> ladders;
    std::vector<GateInfo> gates;
    std::vector<std::pair<int, int>> gateNodes; // node under a gate, the gate
    std::vector<std::pair<zCWaypoint *, zCWaypoint *>> wayLinks; // way net ways linked into the mesh
    std::vector<bool> wetWays;                                   // ... which of them go through deep water
    NavCore::BuildStats stats;
    NavCore::Params params;
};

struct Collected
{
    NavCore::Input input;
    std::vector<LadderInfo> ladders;
    std::vector<GateInfo> gates;
    std::string world;
    int worldTriangles = 0;
    int objectTriangles = 0;
    int objects = 0;
    int boxes = 0;
};

inline std::unique_ptr<State> Current; // the mesh of the world being played, ready to use
inline std::unique_ptr<State> Kept;    // the last one, kept over a load in case the geometry is the same
inline std::shared_ptr<Job> Pending;
inline std::unique_ptr<Collected> PendingInfo;
inline bool CollectedThisLoad = false;

inline float Dot3(const zVEC3 &a, const zVEC3 &b)
{
    return a.n[0] * b.n[0] + a.n[1] * b.n[1] + a.n[2] * b.n[2];
}

inline zVEC3 Cross3(const zVEC3 &a, const zVEC3 &b)
{
    return zVEC3(a.n[1] * b.n[2] - a.n[2] * b.n[1], a.n[2] * b.n[0] - a.n[0] * b.n[2],
                 a.n[0] * b.n[1] - a.n[1] * b.n[0]);
}

inline zVEC3 TransformPoint(const zMAT4 &m, const zVEC3 &p)
{
    return zVEC3(m.v[0].n[0] * p.n[0] + m.v[0].n[1] * p.n[1] + m.v[0].n[2] * p.n[2] + m.v[0].n[3],
                 m.v[1].n[0] * p.n[0] + m.v[1].n[1] * p.n[1] + m.v[1].n[2] * p.n[2] + m.v[1].n[3],
                 m.v[2].n[0] * p.n[0] + m.v[2].n[1] * p.n[1] + m.v[2].n[2] * p.n[2] + m.v[2].n[3]);
}

inline zVEC3 RotateVector(const zMAT4 &m, const zVEC3 &p)
{
    return zVEC3(m.v[0].n[0] * p.n[0] + m.v[0].n[1] * p.n[1] + m.v[0].n[2] * p.n[2],
                 m.v[1].n[0] * p.n[0] + m.v[1].n[1] * p.n[1] + m.v[1].n[2] * p.n[2],
                 m.v[2].n[0] * p.n[0] + m.v[2].n[1] * p.n[1] + m.v[2].n[2] * p.n[2]);
}

// ---------------------------------------------------------------- the hero's limits

// The hero's own movement limits (step height, steepest walkable slope), from the player AI's configuration.
inline NavCore::Params HeroParams()
{
    NavCore::Params params;
    if (!player || !player->anictrl)
        return params;

    const auto &config = player->anictrl->config;
    static bool logged = false;
    if (!logged)
    {
        logged = true;
        YagaLog::Write(std::format(
            "[navmesh] hero config: step {:.1f}, walk angle {:.3f}, slide {:.3f}/{:.3f}, jump low {:.1f} mid {:.1f}, "
            "water knee {:.1f} chest {:.1f}, swim climb out {:.1f}, min space {:.1f}, chasm scan {:.1f}",
            config.zMV_STEP_HEIGHT, config.zMV_MAX_GROUND_ANGLE_WALK, config.zMV_MAX_GROUND_ANGLE_SLIDE,
            config.zMV_MAX_GROUND_ANGLE_SLIDE2, config.zMV_YMAX_JUMPLOW, config.zMV_YMAX_JUMPMID,
            config.zMV_WATER_DEPTH_KNEE, config.zMV_WATER_DEPTH_CHEST, config.zMV_YMAX_SWIM_CLIMB_OUT,
            config.zMV_MIN_SPACE_MOVE_FORWARD, config.zMV_DETECT_CHASM_SCAN_AT));
    }

    // A step the mesh may climb: a bit over the hero's step height. The seams of Gothic's terrain leave lips of 60 to
    // 80 cm that the engine's NPC movement gets over; for a higher one the navigator jumps (the hero's low jump reaches
    // zMV_YMAX_JUMPLOW, 105 cm).
    if (config.zMV_STEP_HEIGHT >= 10.0f && config.zMV_STEP_HEIGHT <= 150.0f)
        params.agentClimb = config.zMV_STEP_HEIGHT * 1.5f;
    if (config.zMV_YMAX_JUMPLOW > 0.0f)
        params.agentClimb = (std::min)(params.agentClimb, config.zMV_YMAX_JUMPLOW - 10.0f);

    // The steepest ground the hero walks up: the walk angle (radians, 45 degrees). Short bumps steeper than that are
    // walkable anyway (see Collector::AreaFor): the hero steps over them.
    float angle = config.zMV_MAX_GROUND_ANGLE_WALK;
    if (angle > 0.0f && angle < 1.6f)
        angle = angle * 180.0f / 3.14159265f;
    if (angle >= 20.0f && angle <= 75.0f)
        params.maxSlope = angle;
    return params;
}

// ---------------------------------------------------------------- collecting the geometry

inline bool IsDoorVisual(zCVisual *visual)
{
    if (!visual)
        return false;
    std::string name = visual->GetVisualName().ToChar();
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
    return name.find("DOOR") != std::string::npos;
}

class Collector
{
  public:
    Collected &out;
    float cosSlope;
    float winding = 1.0f; // +1 when (b-a)x(c-a) points out of the front face, as in the world mesh
    uint64_t worldHash = NavCore::HashSeed;
    uint64_t objectHashSum = 0;

    Collector(Collected &target, const NavCore::Params &params)
        : out(target), cosSlope(std::cos(params.maxSlope * 3.14159265f / 180.0f)), climb(params.agentClimb)
    {
    }

    int AddVertex(const zVEC3 &p)
    {
        out.input.verts.push_back(p.n[0]);
        out.input.verts.push_back(p.n[1]);
        out.input.verts.push_back(p.n[2]);
        return static_cast<int>(out.input.verts.size() / 3) - 1;
    }

    void AddTriangle(const int a, const int b, const int c, const unsigned char area, const bool up)
    {
        out.input.tris.push_back(a);
        out.input.tris.push_back(b);
        out.input.tris.push_back(c);
        out.input.areas.push_back(area);
        out.input.upFacing.push_back(up ? 1 : 0);
        out.input.owners.push_back(owner);
    }

    // Whether the triangle's front side faces up.
    bool FacesUp(const int a, const int b, const int c) const
    {
        const zVEC3 pa = Vertex(a);
        const zVEC3 normal = Cross3(Vertex(b) - pa, Vertex(c) - pa);
        const float length = std::sqrt(Dot3(normal, normal));
        return length > 1e-4f && winding * normal.n[1] / length > 0.05f;
    }

    // The object the next triangles come from (for the OBJ export), or the static world.
    int owner = -1;
    void BeginObject(zCVob *vob, const char *kind)
    {
        zCVisual *visual = vob->GetVisual();
        const zVEC3 p = vob->GetPositionWorld();
        out.input.ownerNames.push_back(std::format("{}:{}@{:.0f},{:.0f},{:.0f}", kind,
                                                   visual ? visual->GetVisualName().ToChar() : "-", p.n[0], p.n[1],
                                                   p.n[2]));
        owner = static_cast<int>(out.input.ownerNames.size()) - 1;
    }

    zVEC3 Vertex(const int index) const
    {
        return zVEC3(out.input.verts[index * 3], out.input.verts[index * 3 + 1], out.input.verts[index * 3 + 2]);
    }

    float climb = 0.0f; // the agent's step: a steeper face no higher than this is a bump the hero steps over

    // Ground by the face's normal: up to the walk angle; steeper only for a bump no higher than a step, facing up.
    unsigned char GroundArea(const float up, const int a, const int b, const int c) const
    {
        if (up >= cosSlope)
            return NavCore::AreaGround;
        if (up > 0.1f)
        {
            const float lowest = (std::min)({Vertex(a).n[1], Vertex(b).n[1], Vertex(c).n[1]});
            const float highest = (std::max)({Vertex(a).n[1], Vertex(b).n[1], Vertex(c).n[1]});
            if (highest - lowest <= climb)
                return NavCore::AreaGround;
        }
        return NavCore::AreaNone;
    }

    // Surface to walk on or swim in, by the face's normal.
    unsigned char AreaOf(const int a, const int b, const int c, const bool water) const
    {
        const zVEC3 pa = Vertex(a);
        zVEC3 normal = Cross3(Vertex(b) - pa, Vertex(c) - pa);
        const float length = std::sqrt(Dot3(normal, normal));
        if (length < 1e-4f)
            return NavCore::AreaNone;
        const float up = winding * normal.n[1] / length;
        if (water)
            return std::abs(up) > 0.5f ? NavCore::AreaWater : NavCore::AreaNone;
        return GroundArea(up, a, b, c);
    }

    std::unordered_map<zCMaterial *, bool> known; // materials already checked by IsFarBackdrop

    bool IsFarBackdrop(zCMaterial *material)
    {
        const auto found = known.find(material);
        if (found != known.end())
            return found->second;
        std::string name = material->GetName().ToChar();
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
        const bool backdrop = name.find("MOUNTAINFAR") != std::string::npos;
        known[material] = backdrop;
        return backdrop;
    }

    // The static mesh's polygons, from the BSP tree the engine collides with. Its list names a polygon once for every
    // leaf the polygon lies in, so repeats are dropped. (The mesh's own polyList is not valid in a loaded world.)
    static std::vector<zCPolygon *> StaticPolygons(zCWorld *world)
    {
        std::vector<zCPolygon *> result;
        zCPolygon **list = world->bspTree.treePolyList;
        const int count = world->bspTree.numPolys;
        if (!list || count <= 0)
            return result;
        std::unordered_set<zCPolygon *> seen;
        seen.reserve(static_cast<size_t>(count));
        for (int i = 0; i < count; ++i)
            if (list[i] && seen.insert(list[i]).second)
                result.push_back(list[i]);
        return result;
    }

    void StaticWorld(zCWorld *world)
    {
        const std::vector<zCPolygon *> list = StaticPolygons(world);
        zCPolygon *const *polys = list.data();
        const int count = static_cast<int>(list.size());
        if (!count)
            return;

        // Which way round the world mesh's faces turn, checked against the engine's own plane normals.
        int agree = 0;
        int disagree = 0;
        for (int i = 0; i < count; ++i)
        {
            zCPolygon *poly = polys[i];
            if (!poly || poly->polyNumVert < 3 || std::abs(poly->polyPlane.normal.n[1]) < 0.5f)
                continue;
            const zVEC3 a = poly->vertex[0]->position;
            const zVEC3 normal = Cross3(poly->vertex[1]->position - a, poly->vertex[2]->position - a);
            (Dot3(normal, poly->polyPlane.normal) >= 0.0f ? agree : disagree)++;
        }
        winding = agree >= disagree ? 1.0f : -1.0f;

        for (int i = 0; i < count; ++i)
        {
            zCPolygon *poly = polys[i];
            if (!poly || poly->polyNumVert < 3)
                continue;
            // Portals and occluders are invisible helpers for the renderer, not walls: an occluder spans the Old Camp
            // gate like a sloping roof, and the hero walks right through it.
            if (poly->flags.portalPoly || poly->flags.ghostOccluder || poly->flags.occluder)
                continue;
            zCMaterial *material = poly->material;
            const bool water = material && material->matGroup == zMAT_GROUP_WATER;
            if (material && material->noCollDet && !water)
                continue;
            // The far-mountain backdrop (materials named ...MOUNTAINFAR...) cuts through paths in places, e.g. the
            // way up from the intro lake runs underneath it. The engine collides with a polygon from the front only,
            // so the hero walks under it; Recast would take it for a low ceiling.
            if (material && IsFarBackdrop(material))
                continue;

            const float up = poly->polyPlane.normal.n[1];
            const int first = static_cast<int>(out.input.verts.size() / 3);
            for (int k = 0; k < poly->polyNumVert; ++k)
            {
                const zVEC3 &p = poly->vertex[k]->position;
                AddVertex(p);
                NavCore::HashBytes(worldHash, p.n, sizeof(float) * 3);
            }
            for (int k = 2; k < poly->polyNumVert; ++k)
            {
                const unsigned char area = water ? (std::abs(up) > 0.5f ? NavCore::AreaWater : NavCore::AreaNone)
                                                 : GroundArea(up, first, first + k - 1, first + k);
                AddTriangle(first, first + k - 1, first + k, area, up > 0.05f);
                ++out.worldTriangles;
            }
        }
    }

    void HashObject(zCVob *vob, zCVisual *visual, const char *kind)
    {
        uint64_t hash = NavCore::HashSeed;
        NavCore::HashBytes(hash, kind, std::strlen(kind));
        const std::string name = visual ? visual->GetVisualName().ToChar() : "";
        NavCore::HashBytes(hash, name.data(), name.size());
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 4; ++c)
            {
                const int quantized = static_cast<int>(std::lround(vob->trafoObjToWorld.v[r].n[c] * 100.0f));
                NavCore::HashBytes(hash, &quantized, sizeof(quantized));
            }
        objectHashSum += hash; // a sum, so the order objects come in (it differs between saves) does not matter
    }

    void ProgMesh(zCVob *vob, zCProgMeshProto *proto)
    {
        const zMAT4 &trafo = vob->trafoObjToWorld;
        const int positions = proto->posList.numInArray;
        if (positions <= 0 || !proto->posList.array)
            return;
        const int first = static_cast<int>(out.input.verts.size() / 3);
        for (int i = 0; i < positions; ++i)
            AddVertex(TransformPoint(trafo, proto->posList.array[i]));

        for (int s = 0; s < proto->numSubMeshes; ++s)
        {
            zCProgMeshProto::zCSubMesh &sub = proto->subMeshList[s];
            zCMaterial *material = sub.material;
            const bool water = material && material->matGroup == zMAT_GROUP_WATER;
            if (material && material->noCollDet && !water)
                continue;
            for (int t = 0; t < sub.triList.numInArray; ++t)
            {
                const zTPMTriangle &tri = sub.triList.array[t];
                int index[3];
                bool valid = true;
                for (int k = 0; k < 3; ++k)
                {
                    const int wedge = tri.wedge[k];
                    if (wedge >= sub.wedgeList.numInArray)
                    {
                        valid = false;
                        break;
                    }
                    const int position = sub.wedgeList.array[wedge].position;
                    if (position >= positions)
                    {
                        valid = false;
                        break;
                    }
                    index[k] = first + position;
                }
                if (!valid)
                    continue;
                AddTriangle(index[0], index[1], index[2], AreaOf(index[0], index[1], index[2], water),
                            FacesUp(index[0], index[1], index[2]));
                ++out.objectTriangles;
            }
        }
        HashObject(vob, proto, "mesh");
        ++out.objects;
    }

    // Furniture and other mobs (models): their box, turned with the object.
    void Box(zCVob *vob)
    {
        const zTBBox3D local = vob->GetBBox3DLocal();
        const zMAT4 &trafo = vob->trafoObjToWorld;
        int corner[8];
        for (int i = 0; i < 8; ++i)
        {
            const zVEC3 p((i & 1) ? local.maxs.n[0] : local.mins.n[0], (i & 2) ? local.maxs.n[1] : local.mins.n[1],
                          (i & 4) ? local.maxs.n[2] : local.mins.n[2]);
            corner[i] = AddVertex(TransformPoint(trafo, p));
        }
        // Faces: corner indices and the face's outward normal in the object's space.
        static const int faces[6][4] = {{0, 1, 3, 2}, {4, 6, 7, 5}, {0, 4, 5, 1},
                                        {2, 3, 7, 6}, {0, 2, 6, 4}, {1, 5, 7, 3}};
        static const float normals[6][3] = {{0, 0, -1}, {0, 0, 1}, {0, -1, 0}, {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}};
        for (int f = 0; f < 6; ++f)
        {
            const zVEC3 normal = RotateVector(trafo, zVEC3(normals[f][0], normals[f][1], normals[f][2]));
            const unsigned char area = normal.n[1] >= cosSlope ? NavCore::AreaGround : NavCore::AreaNone;
            AddTriangle(corner[faces[f][0]], corner[faces[f][1]], corner[faces[f][2]], area, normal.n[1] > 0.05f);
            AddTriangle(corner[faces[f][0]], corner[faces[f][2]], corner[faces[f][3]], area, normal.n[1] > 0.05f);
            out.objectTriangles += 2;
        }
        HashObject(vob, vob->visual, "box");
        ++out.boxes;
    }

    void Gate(zCMover *mover)
    {
        GateInfo gate;
        gate.vob = mover;
        gate.range = mover->bbox3D;
        const zVEC3 position = mover->GetPositionWorld();
        for (int i = 0; i < mover->keyframeList.numInArray; ++i)
        {
            const zVEC3 delta = mover->keyframeList.parray[i].pos - position;
            for (int a = 0; a < 3; ++a)
            {
                gate.range.mins.n[a] = (std::min)(gate.range.mins.n[a], mover->bbox3D.mins.n[a] + delta.n[a]);
                gate.range.maxs.n[a] = (std::max)(gate.range.maxs.n[a], mover->bbox3D.maxs.n[a] + delta.n[a]);
            }
        }
        out.gates.push_back(gate);
    }

    void Object(zCVob *vob)
    {
        if (zDYNAMIC_CAST<oCNpc>(vob) || zDYNAMIC_CAST<oCItem>(vob) || zDYNAMIC_CAST<zCVobLevelCompo>(vob))
            return;
        if (oCMobLadder *ladder = zDYNAMIC_CAST<oCMobLadder>(vob))
        {
            LadderInfo info;
            info.vob = ladder;
            info.box = ladder->bbox3D;
            info.at = ladder->GetAtVectorWorld();
            out.ladders.push_back(info);
            return;
        }
        if (zCMover *mover = zDYNAMIC_CAST<zCMover>(vob))
        {
            Gate(mover);
            return;
        }
        zCVisual *visual = vob->visual;
        if (!visual || !vob->collDetectionDynamic)
            return;
        if (zDYNAMIC_CAST<oCMobDoor>(vob) && IsDoorVisual(visual))
            return;
        if (zCProgMeshProto *proto = zDYNAMIC_CAST<zCProgMeshProto>(visual))
        {
            BeginObject(vob, "mesh");
            ProgMesh(vob, proto);
        }
        else if (zDYNAMIC_CAST<zCModel>(visual))
        {
            BeginObject(vob, "box");
            Box(vob);
        }
        owner = -1;
    }

    void Objects(zCWorld *world)
    {
        std::vector<zCTree<zCVob> *> stack;
        for (zCTree<zCVob> *node = world->globalVobTree.firstChild; node; node = node->next)
            stack.push_back(node);
        while (!stack.empty())
        {
            zCTree<zCVob> *node = stack.back();
            stack.pop_back();
            for (zCTree<zCVob> *child = node->firstChild; child; child = child->next)
                stack.push_back(child);
            if (node->data)
                Object(node->data);
        }
    }
};

// Places the route must keep out of, per world.
inline void AddExcludedBoxes(const std::string &world, std::vector<NavCore::Box> &boxes)
{
#if ENGINE == Engine_G1
    if (world == "WORLD.ZEN")
    {
        // The lake below the intro cliff kills a swimming hero. Only its water is left out, not the shore.
        NavCore::Box lake{{4500.0f, 5100.0f, 36000.0f}, {8650.0f, 5400.0f, 39700.0f}, NavCore::AreaNone,
                          NavCore::AreaWater};
        boxes.push_back(lake);
    }
#endif
}

inline std::unique_ptr<Collected> Collect(zCWorld *world, const std::string &worldName)
{
    auto result = std::make_unique<Collected>();
    result->world = worldName;
    result->input.params = HeroParams();
    Collector collector(*result, result->input.params);
    collector.StaticWorld(world);

    // The world's extent is that of its static mesh; objects far outside it are left out of the build.
    NavCore::Input &input = result->input;
    const size_t staticVerts = input.verts.size() / 3;
    if (staticVerts)
    {
        for (int a = 0; a < 3; ++a)
        {
            input.boundsMin[a] = input.verts[a];
            input.boundsMax[a] = input.verts[a];
        }
        for (size_t i = 0; i < staticVerts; ++i)
            for (int a = 0; a < 3; ++a)
            {
                const float value = input.verts[i * 3 + a];
                if (value < input.boundsMin[a])
                    input.boundsMin[a] = value;
                if (value > input.boundsMax[a])
                    input.boundsMax[a] = value;
            }
        YagaLog::Write(std::format("[navmesh] {} static vertices, the first at {:.0f} {:.0f} {:.0f}", staticVerts,
                                   input.verts[0], input.verts[1], input.verts[2]));
    }
    collector.Objects(world);

    // Gates: their ground gets its own area, so it can be blocked while the gate is down.
    const float margin = result->input.params.agentRadius + 20.0f;
    uint64_t boxHashSum = 0;
    for (const GateInfo &gate : result->gates)
    {
        NavCore::Box box{{gate.range.mins.n[0] - margin, gate.range.mins.n[1] - 200.0f, gate.range.mins.n[2] - margin},
                         {gate.range.maxs.n[0] + margin, gate.range.maxs.n[1], gate.range.maxs.n[2] + margin},
                         NavCore::AreaGate,
                         0};
        result->input.boxes.push_back(box);
        uint64_t hash = NavCore::HashSeed;
        NavCore::HashBytes(hash, &box, sizeof(box));
        boxHashSum += hash;
    }
    AddExcludedBoxes(worldName, result->input.boxes);

    uint64_t hash = NavCore::HashSeed;
    constexpr uint32_t version = 14; // raise when the build changes, so old cache files are not used
    NavCore::HashBytes(hash, &version, sizeof(version));
    NavCore::HashBytes(hash, &collector.worldHash, sizeof(collector.worldHash));
    NavCore::HashBytes(hash, &collector.objectHashSum, sizeof(collector.objectHashSum));
    NavCore::HashBytes(hash, &boxHashSum, sizeof(boxHashSum));
    for (const NavCore::Box &box : result->input.boxes)
        if (box.area != NavCore::AreaGate)
            NavCore::HashBytes(hash, &box, sizeof(box));
    NavCore::HashBytes(hash, &result->input.params, sizeof(result->input.params));
    NavCore::HashBytes(hash, worldName.data(), worldName.size());
    result->input.hash = hash;

    YagaLog::Write(std::format("[navmesh] {}: {} world triangles, {} object triangles from {} objects and {} boxes, "
                               "{} ladders, {} gates, winding {:+.0f}, step {:.0f} cm, slope {:.0f} degrees, "
                               "bounds {:.0f} {:.0f} {:.0f} to {:.0f} {:.0f} {:.0f}",
                               worldName, result->worldTriangles, result->objectTriangles, result->objects,
                               result->boxes, result->ladders.size(), result->gates.size(), collector.winding,
                               input.params.agentClimb, input.params.maxSlope, input.boundsMin[0], input.boundsMin[1],
                               input.boundsMin[2], input.boundsMax[0], input.boundsMax[1], input.boundsMax[2]));
    return result;
}

// ---------------------------------------------------------------- building

inline std::wstring CachePath(const std::string &world)
{
    return ToWString(WaynetPatch::GameFolder() + "\\YAGA\\navmesh\\" + world + ".nav");
}

inline void RunJob(std::shared_ptr<Job> job)
{
    job->mesh = NavCore::Load(job->cachePath, job->input.hash, job->stats);
    if (!job->mesh && !job->cancel.load())
    {
        job->mesh = NavCore::Build(job->input, job->cancel, job->stats);
        if (job->mesh && !job->cancel.load())
            job->saved = NavCore::Save(job->cachePath, job->input.hash, *job->mesh);
    }
    if (job->mesh && !job->cancel.load())
    {
        job->graph = std::make_unique<NavCore::Graph>();
        job->graph->Build(*job->mesh);
    }
    job->done.store(true);
}

// Links each ladder's foot to its top: a spot on the mesh in front of the ladder at its foot, and the walkable spot
// nearest its top.
inline void LinkLadders(State &state)
{
    state.graph.ClearLinks();
    int linked = 0;
    for (size_t i = 0; i < state.ladders.size(); ++i)
    {
        LadderInfo &ladder = state.ladders[i];
        ladder.linked = false;
        zVEC3 at = ladder.at;
        at.n[1] = 0.0f;
        const float atLength = std::sqrt(Dot3(at, at));
        if (atLength > 0.01f)
            at = at * (1.0f / atLength);
        const float cx = (ladder.box.mins.n[0] + ladder.box.maxs.n[0]) * 0.5f;
        const float cz = (ladder.box.mins.n[2] + ladder.box.maxs.n[2]) * 0.5f;
        const float bottomY = ladder.box.mins.n[1];
        const float topY = ladder.box.maxs.n[1];

        int bottomNode = -1;
        float bottom[3];
        for (const float d : {80.0f, -80.0f, 140.0f, -140.0f})
        {
            const float probe[3] = {cx + at.n[0] * d, bottomY + 60.0f, cz + at.n[2] * d};
            const float extents[3] = {70.0f, 120.0f, 70.0f};
            float nearest[3];
            const int node = NavCore::NearestNode(*state.query, state.graph, probe, extents, nearest);
            if (node >= 0 && nearest[1] < bottomY + 150.0f && nearest[1] > bottomY - 120.0f)
            {
                bottomNode = node;
                std::copy(nearest, nearest + 3, bottom);
                break;
            }
        }

        int topNode = -1;
        float top[3];
        for (const float d : {-90.0f, 90.0f, -150.0f, 150.0f, 0.0f})
        {
            const float probe[3] = {cx + at.n[0] * d, topY, cz + at.n[2] * d};
            const float extents[3] = {80.0f, 160.0f, 80.0f};
            float nearest[3];
            const int node = NavCore::NearestNode(*state.query, state.graph, probe, extents, nearest);
            if (node >= 0 && nearest[1] > topY - 220.0f && nearest[1] < topY + 100.0f &&
                (bottomNode < 0 || nearest[1] - bottom[1] > 150.0f))
            {
                topNode = node;
                std::copy(nearest, nearest + 3, top);
                break;
            }
        }

        if (bottomNode < 0 || topNode < 0 || bottomNode == topNode)
        {
            YagaLog::Write(std::format("[navmesh] ladder {} at {:.0f} {:.0f} {:.0f} not linked (foot {}, top {})",
                                       ladder.vob->GetObjectName().ToChar(), cx, bottomY, cz,
                                       bottomNode >= 0 ? "found" : "missing", topNode >= 0 ? "found" : "missing"));
            continue;
        }

        const float cost = (top[1] - bottom[1]) * LadderCostFactor + LadderExtraCost;
        NavCore::Link up{bottomNode, topNode, {bottom[0], bottom[1], bottom[2]}, {top[0], top[1], top[2]}, cost,
                         static_cast<int>(i * 2)};
        NavCore::Link down{topNode, bottomNode, {top[0], top[1], top[2]}, {bottom[0], bottom[1], bottom[2]}, cost,
                           static_cast<int>(i * 2 + 1)};
        state.graph.AddLink(up);
        state.graph.AddLink(down);
        ladder.linked = true;
        ++linked;
    }
    YagaLog::Write(std::format("[navmesh] {} of {} ladders linked", linked, state.ladders.size()));
}

// Link tags from this value on name way net ways (the index into State::wayLinks); below it, ladders.
constexpr int WayTagBase = 1 << 20;

// Depth of the water at a spot (0 = dry), as the navigator measures it: the water surface a downward trace finds,
// minus the ground one that ignores water finds. A bridge or boardwalk over the water counts as dry.
inline float WaterDepth(const zVEC3 &position)
{
    oCWorld *world = ogame->GetGameWorld();
    const zVEC3 origin = position + zVEC3(0.0f, 250.0f, 0.0f);
    const zVEC3 ray(0.0f, -1500.0f, 0.0f);
    const int solid = zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER |
                      zTRACERAY_POLY_IGNORE_TRANSP;
    if (!world->TraceRayNearestHit(origin, ray, player, solid | zTRACERAY_POLY_TEST_WATER))
        return 0.0f;
    if (world->traceRayReport.foundVob)
        return 0.0f;
    zCPolygon *poly = world->traceRayReport.foundPoly;
    if (!poly || !poly->material || poly->material->matGroup != zMAT_GROUP_WATER)
        return 0.0f;
    const float surface = world->traceRayReport.foundIntersection[VY];
    if (!world->TraceRayNearestHit(origin, ray, player, solid))
        return 1500.0f;
    return (std::max)(0.0f, surface - world->traceRayReport.foundIntersection[VY]);
}

// Deeper than a metre anywhere along the line (sampled every 3 m)?
inline bool CrossesDeepWater(const zVEC3 &from, const zVEC3 &to)
{
    const int samples = (std::max)(4, static_cast<int>(DistanceXZ(from, to) / 300.0f));
    for (int i = 1; i <= samples; ++i)
        if (WaterDepth(from + (to - from) * (static_cast<float>(i) / samples)) > 100.0f)
            return true;
    return false;
}

// The game's way net is the road network on top of the mesh. Every way becomes a link the navigator walks like it
// walks the way net, at a lower cost than open ground, so routes keep to the paths NPCs use (the engine's movement is
// known to get through there) and leave them only for the first and last stretch, or where no way leads. Ways also
// bridge what the mesh misses (a lip, a seam in the terrain). Ways with a ladder are left to the ladder links; ways
// the waynet patch marks as avoided are left out.
constexpr float WayCostFactor = 0.5f;

inline void LinkWayNet(State &state)
{
    oCWorld *world = ogame ? ogame->GetGameWorld() : nullptr;
    zCWayNet *wayNet = world ? world->wayNet : nullptr;
    state.wayLinks.clear();
    if (!wayNet)
        return;
    WaynetPatch::EnsureBuilt();

    state.wetWays.clear();
    std::unordered_map<zCWaypoint *, std::pair<int, zVEC3>> nodes;
    const auto nodeOf = [&](zCWaypoint *wp) -> std::pair<int, zVEC3> {
        const auto found = nodes.find(wp);
        if (found != nodes.end())
            return found->second;
        // A waypoint sits half a metre above the ground.
        const zVEC3 position = wp->GetPositionWorld() - zVEC3(0.0f, 50.0f, 0.0f);
        const float extents[3] = {150.0f, 250.0f, 150.0f};
        float nearest[3];
        const int node = NavCore::NearestNode(*state.query, state.graph, position.n, extents, nearest);
        return nodes[wp] = {node, zVEC3(nearest[0], nearest[1], nearest[2])};
    };

    std::vector<std::pair<zCWaypoint *, zCWaypoint *>> candidates;
    for (auto node = wayNet->waylist.next; node; node = node->next)
    {
        zCWay *way = node->data;
        if (!way || !way->left || !way->right || static_cast<oCWay *>(way)->ladder)
            continue;
        candidates.emplace_back(way->left, way->right);
    }
    for (const auto &[from, list] : WaynetPatch::Links)
        for (zCWaypoint *to : list)
            if (from < to)
                candidates.emplace_back(from, to);

    int added = 0;
    for (const auto &[a, b] : candidates)
    {
        if (WaynetPatch::IsAvoided(a, b))
            continue;
        const auto [nodeA, posA] = nodeOf(a);
        const auto [nodeB, posB] = nodeOf(b);
        if (nodeA < 0 || nodeB < 0 || nodeA == nodeB)
            continue;
        const int tag = WayTagBase + static_cast<int>(state.wayLinks.size());
        state.wayLinks.emplace_back(a, b);
        // A way through deep water is swum, which costs as much as swimming on the mesh.
        const bool wet = CrossesDeepWater(a->GetPositionWorld(), b->GetPositionWorld());
        state.wetWays.push_back(wet);
        const float cost = posA.Distance(posB) * (wet ? 10.0f : WayCostFactor);
        state.graph.AddLink({nodeA, nodeB, {posA.n[0], posA.n[1], posA.n[2]}, {posB.n[0], posB.n[1], posB.n[2]}, cost,
                             tag});
        state.graph.AddLink({nodeB, nodeA, {posB.n[0], posB.n[1], posB.n[2]}, {posA.n[0], posA.n[1], posA.n[2]}, cost,
                             tag});
        ++added;
    }
    YagaLog::Write(std::format("[navmesh] {} way net ways linked", added));
}

inline bool Overlaps(const float *min, const float *max, const zTBBox3D &box, const float margin)
{
    return min[0] <= box.maxs.n[0] + margin && max[0] >= box.mins.n[0] - margin &&
           min[2] <= box.maxs.n[2] + margin && max[2] >= box.mins.n[2] - margin;
}

inline void MapGates(State &state)
{
    state.gateNodes.clear();
    for (int node = 0; node < state.graph.Count(); ++node)
    {
        if (state.graph.AreaOf(node) != NavCore::AreaGate)
            continue;
        for (size_t g = 0; g < state.gates.size(); ++g)
            if (Overlaps(state.graph.Min(node), state.graph.Max(node), state.gates[g].range, 60.0f))
                state.gateNodes.emplace_back(node, static_cast<int>(g));
    }
}

// Our links on top of the mesh: ladders, then way net ways where the mesh is cut; islands with both.
inline void LinkAll(State &state)
{
    state.graph.heuristicScale = WayCostFactor;
    LinkLadders(state);
    state.graph.ComputeIslands();
    LinkWayNet(state);
    state.graph.ComputeIslands();
    MapGates(state);
}

inline void LogState(const State &state, const char *how)
{
    int largest = 0;
    for (int i = 0; i < state.graph.IslandCount(); ++i)
        largest = (std::max)(largest, state.graph.IslandSize(i));
    YagaLog::Write(std::format("[navmesh] {} {}: {} polygons in {} tiles, {} islands (largest {}), {} gate polygons, "
                               "{} in {:.0f} ms",
                               how, state.world, state.graph.Count(), state.stats.tiles, state.graph.IslandCount(),
                               largest, state.gateNodes.size(), state.stats.fromCache ? "loaded" : "built",
                               state.stats.milliseconds));
}

inline void Finish()
{
    std::shared_ptr<Job> job = Pending;
    std::unique_ptr<Collected> info = std::move(PendingInfo);
    Pending.reset();
    if (!job || !info)
        return;

    if (!job->mesh || !job->graph)
    {
        YagaLog::Write(std::format("[navmesh] {}: no navigation mesh ({}), the way net is used",
                                   info->world, job->stats.error.empty() ? "failed" : job->stats.error));
        return;
    }

    auto state = std::make_unique<State>();
    state->world = info->world;
    state->hash = job->input.hash;
    state->params = job->input.params;
    state->stats = job->stats;
    state->mesh = std::move(job->mesh);
    state->graph = std::move(*job->graph);
    state->query = NavCore::MakeQuery(*state->mesh);
    if (!state->query)
    {
        YagaLog::Write("[navmesh] could not create the mesh query, the way net is used");
        return;
    }
    state->ladders = std::move(info->ladders);
    state->gates = std::move(info->gates);
    LinkAll(*state);
    if (!job->stats.fromCache)
        YagaLog::Write(std::format("[navmesh] cache {}", job->saved ? "saved" : "could not be saved"));
    LogState(*state, "ready");
    Current = std::move(state);
}

inline void Start(zCWorld *world)
{
    const std::string worldName = WaynetPatch::WorldFileName(static_cast<oCWorld *>(world));
    const auto started = std::chrono::steady_clock::now();
    std::unique_ptr<Collected> info = Collect(world, worldName);
    const double collectMs =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    YagaLog::Write(std::format("[navmesh] geometry collected in {:.0f} ms", collectMs));

    // The same world as before the load: keep the mesh, take the new objects' ladders and gates.
    if (Kept && Kept->hash == info->input.hash && Kept->world == worldName)
    {
        Current = std::move(Kept);
        Current->ladders = std::move(info->ladders);
        Current->gates = std::move(info->gates);
        LinkAll(*Current);
        LogState(*Current, "reused");
        return;
    }
    Kept.reset();

    auto job = std::make_shared<Job>();
    job->input = std::move(info->input);
    job->cachePath = CachePath(worldName);
    info->input = NavCore::Input();
    Pending = job;
    PendingInfo = std::move(info);
    std::thread(RunJob, job).detach();
}

// Called every frame: starts the mesh of a newly loaded world and takes it over when the build is done.
inline void Update()
{
    if (!Settings::NavMesh || !ogame || !player)
        return;
    zCWorld *world = ogame->GetGameWorld();
    if (!world)
        return;

    if (!CollectedThisLoad)
    {
        CollectedThisLoad = true;
        Start(world);
    }
    if (Pending && Pending->done.load())
        Finish();
}

// A world is about to load: the objects the mesh points to go away. The mesh itself is kept for a load of the same
// world.
inline void Reset()
{
    if (Pending)
    {
        Pending->cancel.store(true);
        Pending.reset();
    }
    PendingInfo.reset();
    if (Current)
    {
        Current->ladders.clear();
        Current->gates.clear();
        Current->gateNodes.clear();
        Current->wayLinks.clear();
        Kept = std::move(Current);
    }
    CollectedThisLoad = false;
}

// Throws the mesh away, cache file included, and builds it again.
inline void Rebuild()
{
    std::string world = Current ? Current->world : (Kept ? Kept->world : "");
    Reset();
    Kept.reset();
    if (!world.empty())
        _wremove(CachePath(world).c_str());
    else if (ogame && ogame->GetGameWorld())
        _wremove(CachePath(WaynetPatch::WorldFileName(ogame->GetGameWorld())).c_str());
}

// ---------------------------------------------------------------- queries

inline bool Ready()
{
    return Settings::NavMesh && Current && Current->query;
}

inline bool Building()
{
    return Settings::NavMesh && Pending != nullptr;
}

inline int NodeAt(const zVEC3 &pos, zVEC3 *snapped = nullptr, const float extentXZ = GoalExtentXZ,
                  const float extentY = GoalExtentY)
{
    if (!Ready())
        return -1;
    const float extents[3] = {extentXZ, extentY, extentXZ};
    float nearest[3];
    const int node = NavCore::NearestNode(*Current->query, Current->graph, pos.n, extents, nearest);
    if (node >= 0 && snapped)
        *snapped = zVEC3(nearest[0], nearest[1], nearest[2]);
    return node;
}

inline int IslandOf(const int node)
{
    return Ready() ? Current->graph.Island(node) : -1;
}

inline zVEC3 ClosestPointOn(const int node, const zVEC3 &pos)
{
    float closest[3];
    NavCore::ClosestPoint(*Current->query, Current->graph, node, pos.n, closest);
    return zVEC3(closest[0], closest[1], closest[2]);
}

// Nodes whose centre lies in the sector, nearest to its centre first.
inline std::vector<int> SectorNodes(const SECTOR &sector)
{
    std::vector<int> result;
    if (!Ready())
        return result;
    const NavCore::Graph &graph = Current->graph;
    for (int node = 0; node < graph.Count(); ++node)
    {
        const float *c = graph.Center(node);
        if (SameSector(SectorOf(zVEC3(c[0], c[1], c[2])), sector))
            result.push_back(node);
    }
    const zVEC3 center = SectorCenter(sector);
    std::sort(result.begin(), result.end(), [&](const int a, const int b) {
        const float *ca = graph.Center(a);
        const float *cb = graph.Center(b);
        return DistanceXZ(zVEC3(ca[0], ca[1], ca[2]), center) < DistanceXZ(zVEC3(cb[0], cb[1], cb[2]), center);
    });
    return result;
}

// Whether a gate is down across a node: its present box reaches below head height over the node's floor (its lowest
// point; a large sloping polygon reaches far higher elsewhere).
inline bool GateDown(const GateInfo &gate, const int node)
{
    if (!gate.vob)
        return false;
    const zTBBox3D &box = gate.vob->bbox3D;
    const float *min = Current->graph.Min(node);
    const float *max = Current->graph.Max(node);
    if (!Overlaps(min, max, box, 30.0f))
        return false;
    const float floor = min[1];
    return box.mins.n[1] < floor + Current->params.agentHeight - 10.0f && box.maxs.n[1] > floor + 20.0f;
}

// Whether a link is a way net way through deep water.
inline bool IsWetWay(const int tag)
{
    if (!Ready() || tag < WayTagBase)
        return false;
    const size_t index = static_cast<size_t>(tag - WayTagBase);
    return index < Current->wetWays.size() && Current->wetWays[index];
}

// Nodes to leave out when the hero could not swim to a bank: that bank and the polygons next to it.
inline std::vector<int> BankToBlock(const zVEC3 &bank)
{
    std::vector<int> result;
    const int node = NodeAt(bank, nullptr, 200.0f, 300.0f);
    if (node < 0)
        return result;
    result.push_back(node);
    for (const int neighbour : Current->graph.Neighbours(node))
        if (Current->graph.AreaOf(neighbour) != NavCore::AreaWater)
            result.push_back(neighbour);
    return result;
}

inline oCMobLadder *LadderOfTag(const int tag)
{
    if (!Ready() || tag < 0 || tag >= WayTagBase)
        return nullptr;
    const size_t index = static_cast<size_t>(tag / 2);
    return index < Current->ladders.size() ? Current->ladders[index].vob : nullptr;
}

// A route from a point on the start node to a point on the goal node. blockedNodes and blockedTags are what this
// navigation has learned to avoid; gates that are down are added here.
inline bool FindRoute(const int startNode, const zVEC3 &startPos, const int goalNode, const zVEC3 &goalPos,
                      const std::set<int> &blockedNodes, const std::set<int> &blockedTags,
                      std::vector<NavCore::Corner> &corners, int &closedGates)
{
    corners.clear();
    closedGates = 0;
    if (!Ready() || startNode < 0 || goalNode < 0)
        return false;
    const NavCore::Graph &graph = Current->graph;
    if (graph.Island(startNode) != graph.Island(goalNode))
        return false;

    std::vector<unsigned char> blocked(graph.Count(), 0);
    for (const int node : blockedNodes)
        if (node >= 0 && node < graph.Count() && node != startNode)
            blocked[node] = 1;
    for (const auto &[node, gate] : Current->gateNodes)
        if (node != startNode && GateDown(Current->gates[gate], node))
        {
            blocked[node] = 1;
            ++closedGates;
        }
    const std::vector<int> tags(blockedTags.begin(), blockedTags.end());

    std::vector<NavCore::Graph::Step> path;
    float length = 0.0f;
    // Swimming costs ten times its length: the hero cannot climb out of the water everywhere the mesh meets a bank.
    if (!graph.FindPath(startNode, startPos.n, goalNode, goalPos.n, blocked, tags, 10.0f, path, length))
        return false;
    return NavCore::MakeCorners(*Current->query, graph, path, startPos.n, goalPos.n, corners);
}

enum class Reach
{
    Unknown,   // no mesh, or one of the two points is off it
    Reachable, // length holds the walking distance
    Unreachable
};

// How far the hero walks from one point to another on the mesh (corner to corner, not as the crow flies).
inline Reach RouteLength(const zVEC3 &from, const zVEC3 &to, float &length)
{
    length = 0.0f;
    if (!Ready())
        return Reach::Unknown;
    zVEC3 start;
    zVEC3 goal;
    const int startNode = NodeAt(from, &start);
    const int goalNode = NodeAt(to, &goal);
    if (startNode < 0 || goalNode < 0)
        return Reach::Unknown;
    if (Current->graph.Island(startNode) != Current->graph.Island(goalNode))
        return Reach::Unreachable;
    std::vector<NavCore::Corner> corners;
    int closedGates = 0;
    if (!FindRoute(startNode, start, goalNode, goal, {}, {}, corners, closedGates))
        return Reach::Unreachable;
    zVEC3 previous = start;
    for (const NavCore::Corner &corner : corners)
    {
        const zVEC3 point(corner.pos[0], corner.pos[1], corner.pos[2]);
        length += previous.Distance(point);
        previous = point;
    }
    return Reach::Reachable;
}

// Nodes to leave out after the hero failed to get from where it stands toward a point: the polygons the straight
// line crosses within a few metres, without the one the hero stands on (the next search starts there).
inline std::vector<int> NodesToBlock(const zVEC3 &hero, const zVEC3 &target)
{
    std::vector<int> result;
    zVEC3 snapped;
    const int heroNode = NodeAt(hero, &snapped);
    if (heroNode < 0)
        return result;
    for (const int node : NavCore::NodesAlong(*Current->query, Current->graph, heroNode, snapped.n, target.n, 400.0f))
        if (node != heroNode)
            result.push_back(node);
    if (result.empty())
    {
        const int targetNode = NodeAt(target, nullptr, 150.0f, 200.0f);
        if (targetNode >= 0 && targetNode != heroNode)
            result.push_back(targetNode);
    }
    return result;
}

inline bool Export(const std::wstring &path, const bool withInput)
{
    if (!Ready())
        return false;
    std::unique_ptr<Collected> info;
    if (withInput && ogame && ogame->GetGameWorld())
        info = Collect(ogame->GetGameWorld(), Current->world);
    std::vector<int> islands(Current->graph.Count());
    for (int node = 0; node < Current->graph.Count(); ++node)
        islands[node] = Current->graph.Island(node);
    return NavCore::ExportObj(path, *Current->mesh, info ? &info->input : nullptr, &islands);
}
} // namespace NavMesh
} // namespace GOTHIC_NAMESPACE
