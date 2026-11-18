#pragma once

// The navigation mesh the navigator walks on: built with Recast from the loaded world's collision geometry, searched
// with our own A* over its polygons, turned into corners with Detour's string pulling. Nothing here knows the game
// engine; NavMesh.hpp collects the geometry and owns the mesh of the current world.

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class dtNavMesh;
class dtNavMeshQuery;

namespace NavCore
{
// Area of a mesh polygon (Recast keeps 6 bits; 63 is its walkable area).
enum Area : unsigned char
{
    AreaNone = 0,    // solid, not a surface to walk on
    AreaGate = 20,    // under a gate: blocked while the gate is closed
    AreaShallow = 24, // ground under shallow water: waded, slowly
    AreaWater = 32,   // deep water: swum
    AreaGround = 63, // ground and floors: walked
};

// A box applied to the walkable surface after it is built: gates, and places the route must keep out of.
struct Box
{
    float min[3];
    float max[3];
    unsigned char area;
    unsigned char onlyArea = 0; // when set, only surface of this area is changed
};

// Sizes in world units (cm), angles in degrees.
struct Params
{
    float cellSize = 20.0f;
    float cellHeight = 10.0f;
    float agentHeight = 180.0f;
    float agentRadius = 50.0f; // a margin from cliff edges, where a corner of the route must not sit
    float agentClimb = 50.0f;
    float maxSlope = 45.0f;
    float tileSize = 4800.0f;
    float shallowWater = 100.0f; // water shallower than this is waded through on the ground, not swum
};

struct Input
{
    std::vector<float> verts;         // x y z
    std::vector<int> tris;            // three vertex indices per triangle
    std::vector<unsigned char> areas; // per triangle: AreaGround or AreaWater for a surface to walk on, else AreaNone
    // Per triangle: 1 when its front side faces up. The engine collides with a polygon from the front only, so such a
    // surface is no ceiling for someone below it.
    std::vector<unsigned char> upFacing;
    std::vector<Box> boxes;
    Params params;
    uint64_t hash = 0; // identifies the geometry and the parameters (filled in by the collector)
    // The world's extent (from its static mesh). Objects far outside it (some are parked kilometres below the world)
    // would stretch the height range past what Recast can store.
    float boundsMin[3] = {0.0f, 0.0f, 0.0f};
    float boundsMax[3] = {0.0f, 0.0f, 0.0f};
    // For the OBJ export only: which object each triangle came from (-1 = the static world).
    std::vector<int> owners;
    std::vector<std::string> ownerNames;
};

struct BuildStats
{
    int tiles = 0;
    int polygons = 0;
    double milliseconds = 0.0;
    bool fromCache = false;
    std::string error;
};

struct NavMeshDeleter
{
    void operator()(dtNavMesh *mesh) const;
};
struct QueryDeleter
{
    void operator()(dtNavMeshQuery *query) const;
};
using NavMeshPtr = std::unique_ptr<dtNavMesh, NavMeshDeleter>;
using QueryPtr = std::unique_ptr<dtNavMeshQuery, QueryDeleter>;

// FNV-1a, for the geometry hash.
inline void HashBytes(uint64_t &hash, const void *data, const size_t size)
{
    const unsigned char *bytes = static_cast<const unsigned char *>(data);
    for (size_t i = 0; i < size; ++i)
    {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
}
constexpr uint64_t HashSeed = 14695981039346656037ull;

// Builds the mesh, tiles in parallel. Returns null when cancelled or when nothing walkable was found.
NavMeshPtr Build(const Input &input, const std::atomic<bool> &cancel, BuildStats &stats);
bool Save(const std::wstring &path, uint64_t hash, const dtNavMesh &mesh);
NavMeshPtr Load(const std::wstring &path, uint64_t hash, BuildStats &stats);
QueryPtr MakeQuery(const dtNavMesh &mesh);

// Writes the mesh polygons (and the input triangles, if given) as an OBJ file, for maps drawn offline. islands: the
// island of every polygon in mesh order (the graph's node order), may be null.
bool ExportObj(const std::wstring &path, const dtNavMesh &mesh, const Input *input, const std::vector<int> *islands);

// A link the mesh itself does not have: a ladder, from a point on one polygon to a point on another.
struct Link
{
    int from;
    int to;
    float fromPos[3];
    float toPos[3];
    float cost;
    int tag; // the ladder's index for the collector
};

enum class LegKind
{
    Walk,
    Swim,
    Link // a ladder
};

// A point the hero walks to, and how the leg leading to it is walked.
struct Corner
{
    float pos[3];
    LegKind leg = LegKind::Walk;
    int tag = -1;  // the link's tag on a Link leg
    int node = -1; // the polygon the corner lies on
};

// The polygons of the mesh as a graph: one node per polygon, neighbours from Detour's links, plus our own links.
class Graph
{
  public:
    void Build(const dtNavMesh &mesh);
    void AddLink(const Link &link);
    void ClearLinks();
    // Connected parts, our links counted both ways. Call after the links are added.
    void ComputeIslands();

    int Count() const
    {
        return static_cast<int>(refs.size());
    }
    int NodeOf(uint64_t ref) const;
    uint64_t RefOf(const int node) const
    {
        return refs[node];
    }
    const float *Center(const int node) const
    {
        return &centers[node * 3];
    }
    const float *Min(const int node) const
    {
        return &mins[node * 3];
    }
    const float *Max(const int node) const
    {
        return &maxs[node * 3];
    }
    unsigned char AreaOf(const int node) const
    {
        return areas[node];
    }
    int Island(const int node) const
    {
        return node >= 0 && node < static_cast<int>(islands.size()) ? islands[node] : -1;
    }
    int IslandCount() const
    {
        return islandCount;
    }
    int IslandSize(const int island) const
    {
        return island >= 0 && island < static_cast<int>(islandSizes.size()) ? islandSizes[island] : 0;
    }
    const std::vector<Link> &Links() const
    {
        return links;
    }
    std::vector<int> Neighbours(const int node) const
    {
        if (node < 0 || node >= Count())
            return {};
        return std::vector<int>(neighbours.begin() + firstNeighbour[node],
                                neighbours.begin() + firstNeighbour[node + 1]);
    }
    // Detour's own count of links on the polygon, and the tile it is in.
    void Describe(int node, int &links, int &tileX, int &tileY, int &edges) const;

    struct Step
    {
        int node;
        int link; // the link that led into this node, -1 for a mesh neighbour
    };
    // The cheapest cost per unit of distance of any edge (links can be cheaper than walking), so the A* estimate never
    // overshoots.
    float heuristicScale = 1.0f;
    // A* from the start polygon (at startPos) to the goal polygon (at goalPos). blocked: one flag per node, may be
    // empty; links whose tag is in blockedTags are not used. Water costs waterCost times its length.
    bool FindPath(int start, const float *startPos, int goal, const float *goalPos,
                  const std::vector<unsigned char> &blocked, const std::vector<int> &blockedTags, float waterCost,
                  std::vector<Step> &path, float &length) const;

  private:
    const dtNavMesh *mesh = nullptr;
    std::vector<uint64_t> refs;
    std::vector<float> centers, mins, maxs;
    std::vector<unsigned char> areas;
    std::vector<int> firstNeighbour; // Count() + 1 entries
    std::vector<int> neighbours;
    std::vector<int> tileOffsets; // first node of each tile, -1 for an empty tile
    std::vector<Link> links;
    std::unordered_map<int, std::vector<int>> linksFrom;
    std::vector<int> islands;
    std::vector<int> islandSizes;
    int islandCount = 0;
};

// Corners of a found path: string pulling over each stretch of mesh polygons, with a corner where the area changes
// (so every leg is all ground or all water) and the ends of every link. The hero's own position is not a corner.
bool MakeCorners(const dtNavMeshQuery &query, const Graph &graph, const std::vector<Graph::Step> &path,
                 const float *startPos, const float *goalPos, std::vector<Corner> &corners);

// The nearest polygon to a point within the extents (half sizes), and the closest point on it.
int NearestNode(const dtNavMeshQuery &query, const Graph &graph, const float *pos, const float *extents,
                float *nearest);

// Nodes along the mesh surface from a point on the start node toward a target point, up to maxLength.
std::vector<int> NodesAlong(const dtNavMeshQuery &query, const Graph &graph, int start, const float *from,
                            const float *to, float maxLength);

// The closest point on a node's polygon to a point, with the surface height there.
void ClosestPoint(const dtNavMeshQuery &query, const Graph &graph, int node, const float *pos, float *closest);
} // namespace NavCore
