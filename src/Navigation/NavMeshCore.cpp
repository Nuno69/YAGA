#include "NavMeshCore.h"

#include <DetourAlloc.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <Recast.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <queue>
#include <thread>

namespace NavCore
{
namespace
{
constexpr char CacheMagic[8] = {'Y', 'A', 'G', 'A', 'N', 'A', 'V', '1'};
constexpr uint32_t CacheVersion = 1;
constexpr int MaxWorkers = 6; // the game is a 32-bit process: every worker holds its tile's heightfield

struct TileData
{
    unsigned char *data = nullptr;
    int size = 0;
};

struct CacheHeader
{
    char magic[8];
    uint32_t version;
    uint32_t tileCount;
    uint64_t hash;
    dtNavMeshParams params;
};

struct CacheTile
{
    int32_t x;
    int32_t y;
    int32_t size;
};

float Distance(const float *a, const float *b)
{
    const float dx = a[0] - b[0];
    const float dy = a[1] - b[1];
    const float dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Water no deeper than the limit is waded through: its surface span is dropped, so the ground below is the floor.
// Spans in a column are kept bottom to top.
void RemoveShallowWater(rcHeightfield &heightfield, const int shallowCells)
{
    for (int i = 0; i < heightfield.width * heightfield.height; ++i)
    {
        rcSpan *previous = nullptr;
        rcSpan *span = heightfield.spans[i];
        while (span)
        {
            rcSpan *next = span->next;
            if (span->area == AreaWater && previous &&
                static_cast<int>(span->smax) - static_cast<int>(previous->smax) < shallowCells)
            {
                if (previous->area == AreaGround)
                    previous->area = AreaShallow;
                previous->next = next;
                span->next = heightfield.freelist;
                heightfield.freelist = span;
                span = next;
                continue;
            }
            previous = span;
            span = next;
        }
    }
}

// The engine collides with a polygon from its front side only. A layer made only of surfaces facing up (a rock shelf,
// the far-mountain backdrop) that hangs over a floor lower than the agent's height is therefore no ceiling for that
// floor: the hero walks under it. Such spans are dropped, where the second heightfield (everything but the surfaces
// facing up) has nothing at their height. Water stays.
void RemoveSoftCeilings(rcHeightfield &solid, const rcHeightfield &hard, const int walkableHeight)
{
    for (int i = 0; i < solid.width * solid.height; ++i)
    {
        int floorTop = -1;
        rcSpan *previous = nullptr;
        rcSpan *span = solid.spans[i];
        while (span)
        {
            rcSpan *next = span->next;
            bool soft = false;
            if (floorTop >= 0 && span->area != AreaWater && static_cast<int>(span->smin) - floorTop < walkableHeight)
            {
                soft = true;
                for (const rcSpan *h = hard.spans[i]; h; h = h->next)
                    if (static_cast<int>(h->smin) <= static_cast<int>(span->smax) + 1 &&
                        static_cast<int>(h->smax) + 1 >= static_cast<int>(span->smin))
                    {
                        soft = false;
                        break;
                    }
            }
            if (soft && previous)
            {
                previous->next = next;
                span->next = solid.freelist;
                solid.freelist = span;
                span = next;
                continue;
            }
            // A kept span is the next floor, or an obstacle that ends the open space above the floor. Water is no floor
            // here: a walkway just above a lake is what the hero walks on.
            floorTop = span->area != RC_NULL_AREA && span->area != AreaWater ? static_cast<int>(span->smax) : -1;
            previous = span;
            span = next;
        }
    }
}

// Like rcMarkBoxArea, but can be limited to surface of one area (only the water of a lake, not its shore).
void MarkBox(const Box &box, rcCompactHeightfield &chf)
{
    const int minX = static_cast<int>((box.min[0] - chf.bmin[0]) / chf.cs);
    const int minY = static_cast<int>((box.min[1] - chf.bmin[1]) / chf.ch);
    const int minZ = static_cast<int>((box.min[2] - chf.bmin[2]) / chf.cs);
    const int maxX = static_cast<int>((box.max[0] - chf.bmin[0]) / chf.cs);
    const int maxY = static_cast<int>((box.max[1] - chf.bmin[1]) / chf.ch);
    const int maxZ = static_cast<int>((box.max[2] - chf.bmin[2]) / chf.cs);
    if (maxX < 0 || minX >= chf.width || maxZ < 0 || minZ >= chf.height)
        return;

    for (int z = (std::max)(minZ, 0); z <= (std::min)(maxZ, chf.height - 1); ++z)
        for (int x = (std::max)(minX, 0); x <= (std::min)(maxX, chf.width - 1); ++x)
        {
            const rcCompactCell &cell = chf.cells[x + z * chf.width];
            for (int i = static_cast<int>(cell.index), end = static_cast<int>(cell.index + cell.count); i < end; ++i)
            {
                const rcCompactSpan &span = chf.spans[i];
                if (span.y < minY || span.y > maxY || chf.areas[i] == RC_NULL_AREA)
                    continue;
                if (box.onlyArea && chf.areas[i] != box.onlyArea)
                    continue;
                chf.areas[i] = box.area;
            }
        }
}

struct TileJob
{
    int x;
    int z;
    std::vector<int> triangles;
    float minY = std::numeric_limits<float>::max(); // height range of the tile's triangles
    float maxY = -std::numeric_limits<float>::max();
};

TileData BuildTile(rcContext &ctx, const Input &input, rcConfig config, const float *worldMin, const float *worldMax,
                   const float tileWorld, const TileJob &job)
{
    TileData result;
    if (job.triangles.empty())
        return result;

    // Each tile gets the height range of its own triangles: Recast stores span heights in 13 bits, so the whole
    // world's range at a fine cell height would not fit.
    const Params &params = input.params;
    config.bmin[0] = worldMin[0] + job.x * tileWorld - config.borderSize * config.cs;
    config.bmin[1] = (std::max)(worldMin[1], job.minY) - config.walkableHeight * config.ch;
    config.bmin[2] = worldMin[2] + job.z * tileWorld - config.borderSize * config.cs;
    config.bmax[0] = worldMin[0] + (job.x + 1) * tileWorld + config.borderSize * config.cs;
    config.bmax[1] = (std::min)(worldMax[1], job.maxY) + config.walkableHeight * config.ch;
    config.bmax[2] = worldMin[2] + (job.z + 1) * tileWorld + config.borderSize * config.cs;
    if ((config.bmax[1] - config.bmin[1]) / config.ch >= RC_SPAN_MAX_HEIGHT)
        config.bmax[1] = config.bmin[1] + (RC_SPAN_MAX_HEIGHT - 1) * config.ch;

    std::vector<int> tris;
    std::vector<unsigned char> areas;
    std::vector<int> hardTris; // everything but the surfaces facing up
    std::vector<unsigned char> hardAreas;
    tris.reserve(job.triangles.size() * 3);
    areas.reserve(job.triangles.size());
    for (const int t : job.triangles)
    {
        tris.insert(tris.end(), &input.tris[t * 3], &input.tris[t * 3] + 3);
        areas.push_back(input.areas[t]);
        if (t >= static_cast<int>(input.upFacing.size()) || !input.upFacing[t])
        {
            hardTris.insert(hardTris.end(), &input.tris[t * 3], &input.tris[t * 3] + 3);
            hardAreas.push_back(1);
        }
    }

    rcHeightfield *solid = rcAllocHeightfield();
    rcCompactHeightfield *chf = nullptr;
    rcContourSet *contours = nullptr;
    rcPolyMesh *polyMesh = nullptr;
    rcPolyMeshDetail *detailMesh = nullptr;
    const auto cleanup = [&]() {
        rcFreeHeightField(solid);
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(contours);
        rcFreePolyMesh(polyMesh);
        rcFreePolyMeshDetail(detailMesh);
    };

    const int vertCount = static_cast<int>(input.verts.size() / 3);
    if (!solid ||
        !rcCreateHeightfield(&ctx, *solid, config.width, config.height, config.bmin, config.bmax, config.cs,
                             config.ch) ||
        !rcRasterizeTriangles(&ctx, input.verts.data(), vertCount, tris.data(), areas.data(),
                              static_cast<int>(areas.size()), *solid, config.walkableClimb))
    {
        cleanup();
        return result;
    }

    RemoveShallowWater(*solid, static_cast<int>(std::ceil(params.shallowWater / config.ch)));
    {
        rcHeightfield *hard = rcAllocHeightfield();
        if (hard &&
            rcCreateHeightfield(&ctx, *hard, config.width, config.height, config.bmin, config.bmax, config.cs,
                                config.ch) &&
            (hardAreas.empty() ||
             rcRasterizeTriangles(&ctx, input.verts.data(), vertCount, hardTris.data(), hardAreas.data(),
                                  static_cast<int>(hardAreas.size()), *hard, config.walkableClimb)))
            RemoveSoftCeilings(*solid, *hard, config.walkableHeight);
        rcFreeHeightField(hard);
    }
    rcFilterLowHangingWalkableObstacles(&ctx, config.walkableClimb, *solid);
    rcFilterLedgeSpans(&ctx, config.walkableHeight, config.walkableClimb, *solid);
    rcFilterWalkableLowHeightSpans(&ctx, config.walkableHeight, *solid);

    chf = rcAllocCompactHeightfield();
    if (!chf || !rcBuildCompactHeightfield(&ctx, config.walkableHeight, config.walkableClimb, *solid, *chf))
    {
        cleanup();
        return result;
    }
    rcFreeHeightField(solid);
    solid = nullptr;

    if (!rcErodeWalkableArea(&ctx, config.walkableRadius, *chf))
    {
        cleanup();
        return result;
    }
    for (const Box &box : input.boxes)
        MarkBox(box, *chf);

    if (!rcBuildDistanceField(&ctx, *chf) ||
        !rcBuildRegions(&ctx, *chf, config.borderSize, config.minRegionArea, config.mergeRegionArea))
    {
        cleanup();
        return result;
    }

    contours = rcAllocContourSet();
    if (!contours ||
        !rcBuildContours(&ctx, *chf, config.maxSimplificationError, config.maxEdgeLen, *contours) ||
        contours->nconts == 0)
    {
        cleanup();
        return result;
    }

    polyMesh = rcAllocPolyMesh();
    detailMesh = rcAllocPolyMeshDetail();
    if (!polyMesh || !detailMesh || !rcBuildPolyMesh(&ctx, *contours, config.maxVertsPerPoly, *polyMesh) ||
        !rcBuildPolyMeshDetail(&ctx, *polyMesh, *chf, config.detailSampleDist, config.detailSampleMaxError,
                               *detailMesh) ||
        polyMesh->npolys == 0 || polyMesh->nverts >= 0xffff)
    {
        cleanup();
        return result;
    }

    for (int i = 0; i < polyMesh->npolys; ++i)
        polyMesh->flags[i] = 1;

    dtNavMeshCreateParams create;
    std::memset(&create, 0, sizeof(create));
    create.verts = polyMesh->verts;
    create.vertCount = polyMesh->nverts;
    create.polys = polyMesh->polys;
    create.polyAreas = polyMesh->areas;
    create.polyFlags = polyMesh->flags;
    create.polyCount = polyMesh->npolys;
    create.nvp = polyMesh->nvp;
    create.detailMeshes = detailMesh->meshes;
    create.detailVerts = detailMesh->verts;
    create.detailVertsCount = detailMesh->nverts;
    create.detailTris = detailMesh->tris;
    create.detailTriCount = detailMesh->ntris;
    create.walkableHeight = params.agentHeight;
    create.walkableRadius = params.agentRadius;
    create.walkableClimb = params.agentClimb;
    create.tileX = job.x;
    create.tileY = job.z;
    create.tileLayer = 0;
    rcVcopy(create.bmin, polyMesh->bmin);
    rcVcopy(create.bmax, polyMesh->bmax);
    create.cs = config.cs;
    create.ch = config.ch;
    create.buildBvTree = true;

    if (!dtCreateNavMeshData(&create, &result.data, &result.size))
    {
        result.data = nullptr;
        result.size = 0;
    }
    cleanup();
    return result;
}
} // namespace

void NavMeshDeleter::operator()(dtNavMesh *mesh) const
{
    dtFreeNavMesh(mesh);
}

void QueryDeleter::operator()(dtNavMeshQuery *query) const
{
    dtFreeNavMeshQuery(query);
}

NavMeshPtr Build(const Input &input, const std::atomic<bool> &cancel, BuildStats &stats)
{
    const auto started = std::chrono::steady_clock::now();
    stats = BuildStats();
    const Params &params = input.params;
    const int vertCount = static_cast<int>(input.verts.size() / 3);
    const int triCount = static_cast<int>(input.tris.size() / 3);
    if (vertCount == 0 || triCount == 0)
    {
        stats.error = "no geometry";
        return nullptr;
    }

    float worldMin[3];
    float worldMax[3];
    if (input.boundsMax[0] > input.boundsMin[0] && input.boundsMax[1] > input.boundsMin[1] &&
        input.boundsMax[2] > input.boundsMin[2])
    {
        rcVcopy(worldMin, input.boundsMin);
        rcVcopy(worldMax, input.boundsMax);
    }
    else
        rcCalcBounds(input.verts.data(), vertCount, worldMin, worldMax);

    rcConfig config;
    std::memset(&config, 0, sizeof(config));
    config.cs = params.cellSize;
    config.ch = params.cellHeight;
    config.walkableSlopeAngle = params.maxSlope;
    config.walkableHeight = static_cast<int>(std::ceil(params.agentHeight / config.ch));
    config.walkableClimb = static_cast<int>(std::floor(params.agentClimb / config.ch));
    config.walkableRadius = static_cast<int>(std::ceil(params.agentRadius / config.cs));
    config.maxEdgeLen = static_cast<int>(1200.0f / config.cs);
    config.maxSimplificationError = 1.3f;
    config.minRegionArea = 8 * 8;
    config.mergeRegionArea = 20 * 20;
    config.maxVertsPerPoly = DT_VERTS_PER_POLYGON;
    config.detailSampleDist = config.cs * 6.0f;
    config.detailSampleMaxError = config.ch;
    config.tileSize = static_cast<int>(params.tileSize / config.cs);
    config.borderSize = config.walkableRadius + 3;
    config.width = config.tileSize + config.borderSize * 2;
    config.height = config.tileSize + config.borderSize * 2;

    const float tileWorld = config.tileSize * config.cs;
    const int tilesX = static_cast<int>(std::ceil((worldMax[0] - worldMin[0]) / tileWorld));
    const int tilesZ = static_cast<int>(std::ceil((worldMax[2] - worldMin[2]) / tileWorld));
    if (tilesX <= 0 || tilesZ <= 0)
    {
        stats.error = "empty bounds";
        return nullptr;
    }

    // Every triangle goes to each tile it touches, border included.
    std::vector<TileJob> jobs(static_cast<size_t>(tilesX) * tilesZ);
    for (int z = 0; z < tilesZ; ++z)
        for (int x = 0; x < tilesX; ++x)
        {
            jobs[z * tilesX + x].x = x;
            jobs[z * tilesX + x].z = z;
        }
    const float border = config.borderSize * config.cs;
    for (int t = 0; t < triCount; ++t)
    {
        const float *a = &input.verts[input.tris[t * 3] * 3];
        const float *b = &input.verts[input.tris[t * 3 + 1] * 3];
        const float *c = &input.verts[input.tris[t * 3 + 2] * 3];
        const float minX = (std::min)({a[0], b[0], c[0]}) - border;
        const float maxX = (std::max)({a[0], b[0], c[0]}) + border;
        const float minZ = (std::min)({a[2], b[2], c[2]}) - border;
        const float maxZ = (std::max)({a[2], b[2], c[2]}) + border;
        const float minY = (std::min)({a[1], b[1], c[1]});
        const float maxY = (std::max)({a[1], b[1], c[1]});
        if (maxY < worldMin[1] || minY > worldMax[1])
            continue;
        const int x0 = (std::max)(0, static_cast<int>(std::floor((minX - worldMin[0]) / tileWorld)));
        const int x1 = (std::min)(tilesX - 1, static_cast<int>(std::floor((maxX - worldMin[0]) / tileWorld)));
        const int z0 = (std::max)(0, static_cast<int>(std::floor((minZ - worldMin[2]) / tileWorld)));
        const int z1 = (std::min)(tilesZ - 1, static_cast<int>(std::floor((maxZ - worldMin[2]) / tileWorld)));
        for (int z = z0; z <= z1; ++z)
            for (int x = x0; x <= x1; ++x)
            {
                TileJob &job = jobs[z * tilesX + x];
                job.triangles.push_back(t);
                job.minY = (std::min)(job.minY, minY);
                job.maxY = (std::max)(job.maxY, maxY);
            }
    }

    std::vector<TileData> tiles(jobs.size());
    std::atomic<int> next{0};
    const auto worker = [&]() {
        rcContext ctx(false);
        while (!cancel.load())
        {
            const int i = next.fetch_add(1);
            if (i >= static_cast<int>(jobs.size()))
                break;
            tiles[i] = BuildTile(ctx, input, config, worldMin, worldMax, tileWorld, jobs[i]);
            jobs[i].triangles = std::vector<int>(); // free memory as we go
        }
    };
    const unsigned hardware = std::thread::hardware_concurrency();
    const int workers = std::clamp(static_cast<int>(hardware > 1 ? hardware - 1 : 1), 1, MaxWorkers);
    std::vector<std::thread> threads;
    for (int i = 0; i < workers; ++i)
        threads.emplace_back(worker);
    for (std::thread &thread : threads)
        thread.join();

    if (cancel.load())
    {
        for (TileData &tile : tiles)
            dtFree(tile.data);
        stats.error = "cancelled";
        return nullptr;
    }

    dtNavMeshParams meshParams;
    std::memset(&meshParams, 0, sizeof(meshParams));
    rcVcopy(meshParams.orig, worldMin);
    meshParams.tileWidth = tileWorld;
    meshParams.tileHeight = tileWorld;
    meshParams.maxTiles = tilesX * tilesZ;
    meshParams.maxPolys = 1 << 16;

    NavMeshPtr mesh(dtAllocNavMesh());
    if (!mesh || dtStatusFailed(mesh->init(&meshParams)))
    {
        for (TileData &tile : tiles)
            dtFree(tile.data);
        stats.error = "navmesh init failed";
        return nullptr;
    }

    for (TileData &tile : tiles)
    {
        if (!tile.data)
            continue;
        if (dtStatusFailed(mesh->addTile(tile.data, tile.size, DT_TILE_FREE_DATA, 0, nullptr)))
        {
            dtFree(tile.data);
            continue;
        }
        const dtMeshHeader *header = reinterpret_cast<const dtMeshHeader *>(tile.data);
        ++stats.tiles;
        stats.polygons += header->polyCount;
    }

    stats.milliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    if (stats.tiles == 0)
    {
        stats.error = "nothing walkable";
        return nullptr;
    }
    return mesh;
}

bool Save(const std::wstring &path, const uint64_t hash, const dtNavMesh &mesh)
{
    std::error_code error;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), error);

    FILE *file = nullptr;
    if (_wfopen_s(&file, path.c_str(), L"wb") != 0 || !file)
        return false;

    CacheHeader header;
    std::memcpy(header.magic, CacheMagic, sizeof(CacheMagic));
    header.version = CacheVersion;
    header.hash = hash;
    header.tileCount = 0;
    std::memcpy(&header.params, mesh.getParams(), sizeof(dtNavMeshParams));
    for (int i = 0; i < mesh.getMaxTiles(); ++i)
    {
        const dtMeshTile *tile = mesh.getTile(i);
        if (tile && tile->header && tile->dataSize)
            ++header.tileCount;
    }

    bool ok = fwrite(&header, sizeof(header), 1, file) == 1;
    for (int i = 0; ok && i < mesh.getMaxTiles(); ++i)
    {
        const dtMeshTile *tile = mesh.getTile(i);
        if (!tile || !tile->header || !tile->dataSize)
            continue;
        CacheTile entry{tile->header->x, tile->header->y, tile->dataSize};
        ok = fwrite(&entry, sizeof(entry), 1, file) == 1 &&
             fwrite(tile->data, static_cast<size_t>(tile->dataSize), 1, file) == 1;
    }
    fclose(file);
    if (!ok)
        std::filesystem::remove(std::filesystem::path(path), error);
    return ok;
}

NavMeshPtr Load(const std::wstring &path, const uint64_t hash, BuildStats &stats)
{
    const auto started = std::chrono::steady_clock::now();
    stats = BuildStats();
    FILE *file = nullptr;
    if (_wfopen_s(&file, path.c_str(), L"rb") != 0 || !file)
        return nullptr;

    CacheHeader header;
    if (fread(&header, sizeof(header), 1, file) != 1 || std::memcmp(header.magic, CacheMagic, sizeof(CacheMagic)) ||
        header.version != CacheVersion || header.hash != hash)
    {
        fclose(file);
        return nullptr;
    }

    NavMeshPtr mesh(dtAllocNavMesh());
    if (!mesh || dtStatusFailed(mesh->init(&header.params)))
    {
        fclose(file);
        return nullptr;
    }

    for (uint32_t i = 0; i < header.tileCount; ++i)
    {
        CacheTile entry;
        if (fread(&entry, sizeof(entry), 1, file) != 1 || entry.size <= 0)
        {
            fclose(file);
            return nullptr;
        }
        unsigned char *data = static_cast<unsigned char *>(dtAlloc(entry.size, DT_ALLOC_PERM));
        if (!data || fread(data, static_cast<size_t>(entry.size), 1, file) != 1 ||
            dtStatusFailed(mesh->addTile(data, entry.size, DT_TILE_FREE_DATA, 0, nullptr)))
        {
            dtFree(data);
            fclose(file);
            return nullptr;
        }
        ++stats.tiles;
        stats.polygons += reinterpret_cast<const dtMeshHeader *>(data)->polyCount;
    }
    fclose(file);
    stats.fromCache = true;
    stats.milliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    return mesh;
}

QueryPtr MakeQuery(const dtNavMesh &mesh)
{
    QueryPtr query(dtAllocNavMeshQuery());
    if (!query || dtStatusFailed(query->init(&mesh, 2048)))
        return nullptr;
    return query;
}

bool ExportObj(const std::wstring &path, const dtNavMesh &mesh, const Input *input, const std::vector<int> *islands)
{
    FILE *file = nullptr;
    if (_wfopen_s(&file, path.c_str(), L"w") != 0 || !file)
        return false;

    int base = 1;
    size_t node = 0;
    fprintf(file, "o navmesh\n");
    for (int i = 0; i < mesh.getMaxTiles(); ++i)
    {
        const dtMeshTile *tile = mesh.getTile(i);
        if (!tile || !tile->header)
            continue;
        for (int v = 0; v < tile->header->vertCount; ++v)
            fprintf(file, "v %.1f %.1f %.1f\n", tile->verts[v * 3], tile->verts[v * 3 + 1], tile->verts[v * 3 + 2]);
        for (int p = 0; p < tile->header->polyCount; ++p)
        {
            const dtPoly &poly = tile->polys[p];
            const int island = islands && node < islands->size() ? (*islands)[node] : -1;
            ++node;
            fprintf(file, "# area %d island %d\nf", poly.getArea(), island);
            for (int k = 0; k < poly.vertCount; ++k)
                fprintf(file, " %d", base + poly.verts[k]);
            fprintf(file, "\n");
        }
        base += tile->header->vertCount;
    }

    if (input)
    {
        fprintf(file, "o input\n");
        for (size_t v = 0; v + 2 < input->verts.size(); v += 3)
            fprintf(file, "v %.1f %.1f %.1f\n", input->verts[v], input->verts[v + 1], input->verts[v + 2]);
        int owner = -2;
        for (size_t t = 0; t + 2 < input->tris.size(); t += 3)
        {
            const size_t triangle = t / 3;
            const int current = triangle < input->owners.size() ? input->owners[triangle] : -1;
            if (current != owner)
            {
                owner = current;
                const std::string name = owner >= 0 && owner < static_cast<int>(input->ownerNames.size())
                                             ? input->ownerNames[owner]
                                             : std::string("static");
                fprintf(file, "g %s area %d\n", name.c_str(), input->areas[triangle]);
            }
            fprintf(file, "f %d %d %d\n", base + input->tris[t], base + input->tris[t + 1], base + input->tris[t + 2]);
        }
    }
    fclose(file);
    return true;
}

// ---------------------------------------------------------------- graph

void Graph::Build(const dtNavMesh &navMesh)
{
    mesh = &navMesh;
    refs.clear();
    centers.clear();
    mins.clear();
    maxs.clear();
    areas.clear();
    firstNeighbour.clear();
    neighbours.clear();
    links.clear();
    linksFrom.clear();
    islands.clear();
    islandSizes.clear();
    islandCount = 0;

    const int maxTiles = navMesh.getMaxTiles();
    tileOffsets.assign(maxTiles, -1);
    for (int t = 0; t < maxTiles; ++t)
    {
        const dtMeshTile *tile = navMesh.getTile(t);
        if (!tile || !tile->header)
            continue;
        tileOffsets[t] = static_cast<int>(refs.size());
        const dtPolyRef base = navMesh.getPolyRefBase(tile);
        for (int p = 0; p < tile->header->polyCount; ++p)
        {
            const dtPoly &poly = tile->polys[p];
            refs.push_back(base | static_cast<dtPolyRef>(p));
            float center[3] = {0.0f, 0.0f, 0.0f};
            float low[3] = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max()};
            float high[3] = {-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
                             -std::numeric_limits<float>::max()};
            for (int k = 0; k < poly.vertCount; ++k)
            {
                const float *v = &tile->verts[poly.verts[k] * 3];
                for (int a = 0; a < 3; ++a)
                {
                    center[a] += v[a];
                    low[a] = (std::min)(low[a], v[a]);
                    high[a] = (std::max)(high[a], v[a]);
                }
            }
            for (int a = 0; a < 3; ++a)
            {
                centers.push_back(center[a] / (std::max)(1, static_cast<int>(poly.vertCount)));
                mins.push_back(low[a]);
                maxs.push_back(high[a]);
            }
            areas.push_back(poly.getArea());
        }
    }

    firstNeighbour.resize(refs.size() + 1);
    for (size_t i = 0; i < refs.size(); ++i)
    {
        firstNeighbour[i] = static_cast<int>(neighbours.size());
        const dtMeshTile *tile = nullptr;
        const dtPoly *poly = nullptr;
        navMesh.getTileAndPolyByRefUnsafe(refs[i], &tile, &poly);
        for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
        {
            const int neighbour = NodeOf(tile->links[k].ref);
            if (neighbour >= 0)
                neighbours.push_back(neighbour);
        }
    }
    firstNeighbour[refs.size()] = static_cast<int>(neighbours.size());
}

void Graph::Describe(const int node, int &linkCount, int &tileX, int &tileY, int &edges) const
{
    linkCount = 0;
    tileX = tileY = edges = -1;
    if (!mesh || node < 0 || node >= Count())
        return;
    const dtMeshTile *tile = nullptr;
    const dtPoly *poly = nullptr;
    mesh->getTileAndPolyByRefUnsafe(refs[node], &tile, &poly);
    tileX = tile->header->x;
    tileY = tile->header->y;
    edges = poly->vertCount;
    for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
        ++linkCount;
}

int Graph::NodeOf(const uint64_t ref) const
{
    if (!mesh || !ref)
        return -1;
    unsigned int salt = 0;
    unsigned int tile = 0;
    unsigned int poly = 0;
    mesh->decodePolyId(ref, salt, tile, poly);
    if (tile >= tileOffsets.size() || tileOffsets[tile] < 0)
        return -1;
    const int node = tileOffsets[tile] + static_cast<int>(poly);
    return node < Count() && refs[node] == ref ? node : -1;
}

void Graph::AddLink(const Link &link)
{
    if (link.from < 0 || link.to < 0 || link.from >= Count() || link.to >= Count())
        return;
    linksFrom[link.from].push_back(static_cast<int>(links.size()));
    links.push_back(link);
}

void Graph::ClearLinks()
{
    links.clear();
    linksFrom.clear();
}

void Graph::ComputeIslands()
{
    const int count = Count();
    std::vector<int> parent(count);
    for (int i = 0; i < count; ++i)
        parent[i] = i;
    const auto find = [&](int x) {
        while (parent[x] != x)
        {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }
        return x;
    };
    const auto unite = [&](const int a, const int b) {
        const int ra = find(a);
        const int rb = find(b);
        if (ra != rb)
            parent[ra] = rb;
    };
    for (int i = 0; i < count; ++i)
        for (int k = firstNeighbour[i]; k < firstNeighbour[i + 1]; ++k)
            unite(i, neighbours[k]);
    for (const Link &link : links)
        unite(link.from, link.to);

    islands.assign(count, -1);
    std::unordered_map<int, int> ids;
    islandSizes.clear();
    for (int i = 0; i < count; ++i)
    {
        const int root = find(i);
        auto found = ids.find(root);
        if (found == ids.end())
        {
            found = ids.emplace(root, static_cast<int>(islandSizes.size())).first;
            islandSizes.push_back(0);
        }
        islands[i] = found->second;
        ++islandSizes[found->second];
    }
    islandCount = static_cast<int>(islandSizes.size());
}

bool Graph::FindPath(const int start, const float *startPos, const int goal, const float *goalPos,
                     const std::vector<unsigned char> &blocked, const std::vector<int> &blockedTags,
                     const float waterCost, std::vector<Step> &path, float &length) const
{
    path.clear();
    length = 0.0f;
    const int count = Count();
    if (start < 0 || goal < 0 || start >= count || goal >= count)
        return false;
    const auto isBlocked = [&](const int node) {
        return node < static_cast<int>(blocked.size()) && blocked[node];
    };
    if (isBlocked(goal))
        return false;

    constexpr float Infinite = std::numeric_limits<float>::max();
    std::vector<float> cost(count, Infinite);
    std::vector<int> parent(count, -1);
    std::vector<int> via(count, -1);
    std::vector<unsigned char> closed(count, 0);
    const auto position = [&](const int node) { return node == start ? startPos : Center(node); };
    // Swimming costs waterCost times the distance, wading through shallow water three times.
    const auto factor = [&](const int a, const int b) {
        if (areas[a] == AreaWater || areas[b] == AreaWater)
            return waterCost;
        if (areas[a] == AreaShallow || areas[b] == AreaShallow)
            return 3.0f;
        return 1.0f;
    };

    using Item = std::pair<float, int>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> open;
    cost[start] = 0.0f;
    open.push({Distance(startPos, goalPos) * heuristicScale, start});
    while (!open.empty())
    {
        const int node = open.top().second;
        open.pop();
        if (closed[node])
            continue;
        closed[node] = 1;
        if (node == goal)
            break;

        const float *here = position(node);
        const auto relax = [&](const int next, const float stepCost, const int link) {
            if (closed[next] || (isBlocked(next) && next != goal))
                return;
            const float total = cost[node] + stepCost;
            if (total < cost[next])
            {
                cost[next] = total;
                parent[next] = node;
                via[next] = link;
                const float *there = next == goal ? goalPos : Center(next);
                open.push({total + Distance(there, goalPos) * heuristicScale, next});
            }
        };

        for (int k = firstNeighbour[node]; k < firstNeighbour[node + 1]; ++k)
        {
            const int next = neighbours[k];
            const float *there = next == goal ? goalPos : Center(next);
            relax(next, Distance(here, there) * factor(node, next), -1);
        }
        const auto links_ = linksFrom.find(node);
        if (links_ != linksFrom.end())
            for (const int index : links_->second)
            {
                const Link &link = links[index];
                if (std::find(blockedTags.begin(), blockedTags.end(), link.tag) != blockedTags.end())
                    continue;
                relax(link.to, Distance(here, link.fromPos) + link.cost, index);
            }
    }

    if (cost[goal] == Infinite)
        return false;
    length = cost[goal];
    for (int node = goal; node >= 0; node = node == start ? -1 : parent[node])
        path.push_back({node, via[node]});
    std::reverse(path.begin(), path.end());
    path.front().link = -1;
    return true;
}

// ---------------------------------------------------------------- queries

void ClosestPoint(const dtNavMeshQuery &query, const Graph &graph, const int node, const float *pos, float *closest)
{
    rcVcopy(closest, pos);
    if (node < 0)
        return;
    float result[3];
    if (dtStatusSucceed(query.closestPointOnPoly(graph.RefOf(node), pos, result, nullptr)))
        rcVcopy(closest, result);
}

int NearestNode(const dtNavMeshQuery &query, const Graph &graph, const float *pos, const float *extents,
                float *nearest)
{
    dtQueryFilter filter;
    dtPolyRef ref = 0;
    float point[3];
    rcVcopy(point, pos);
    if (dtStatusFailed(query.findNearestPoly(pos, extents, &filter, &ref, point)) || !ref)
        return -1;
    if (nearest)
        rcVcopy(nearest, point);
    return graph.NodeOf(ref);
}

std::vector<int> NodesAlong(const dtNavMeshQuery &query, const Graph &graph, const int start, const float *from,
                            const float *to, const float maxLength)
{
    std::vector<int> result;
    if (start < 0)
        return result;
    float end[3];
    rcVcopy(end, to);
    const float length = Distance(from, to);
    if (length > maxLength && length > 0.0f)
        for (int a = 0; a < 3; ++a)
            end[a] = from[a] + (to[a] - from[a]) * (maxLength / length);

    dtQueryFilter filter;
    float hit = 0.0f;
    float normal[3];
    dtPolyRef visited[256];
    int count = 0;
    query.raycast(graph.RefOf(start), from, end, &filter, &hit, normal, visited, &count, 256);
    for (int i = 0; i < count; ++i)
    {
        const int node = graph.NodeOf(visited[i]);
        if (node >= 0)
            result.push_back(node);
    }
    return result;
}

bool MakeCorners(const dtNavMeshQuery &query, const Graph &graph, const std::vector<Graph::Step> &path,
                 const float *startPos, const float *goalPos, std::vector<Corner> &corners)
{
    corners.clear();
    if (path.empty())
        return false;

    float segmentStart[3];
    rcVcopy(segmentStart, startPos);
    float previous[3];
    rcVcopy(previous, startPos);

    size_t first = 0;
    while (first < path.size())
    {
        size_t last = first;
        while (last + 1 < path.size() && path[last + 1].link < 0)
            ++last;
        const bool final = last + 1 >= path.size();
        const Link *link = final ? nullptr : &graph.Links()[path[last + 1].link];

        std::vector<dtPolyRef> corridor;
        for (size_t i = first; i <= last; ++i)
            corridor.push_back(graph.RefOf(path[i].node));

        float a[3];
        float b[3];
        ClosestPoint(query, graph, path[first].node, segmentStart, a);
        ClosestPoint(query, graph, path[last].node, final ? goalPos : link->fromPos, b);

        const int maxPoints = static_cast<int>(corridor.size()) * 3 + 8;
        std::vector<float> points(static_cast<size_t>(maxPoints) * 3);
        std::vector<unsigned char> flags(maxPoints);
        std::vector<dtPolyRef> pointRefs(maxPoints);
        int count = 0;
        query.findStraightPath(a, b, corridor.data(), static_cast<int>(corridor.size()), points.data(), flags.data(),
                               pointRefs.data(), &count, maxPoints, DT_STRAIGHTPATH_AREA_CROSSINGS);
        if (count < 2)
        {
            // Start and end on the same spot, or Detour failed: head straight for the end.
            count = 2;
            rcVcopy(&points[0], a);
            rcVcopy(&points[3], b);
            pointRefs[0] = corridor.front();
            pointRefs[1] = corridor.back();
        }

        for (int k = 1; k < count; ++k)
        {
            const float *point = &points[k * 3];
            const bool segmentEnd = k == count - 1;
            // Corners this close to the previous one add nothing but a stop.
            if (!segmentEnd && Distance(point, previous) < 30.0f)
                continue;
            Corner corner;
            rcVcopy(corner.pos, point);
            const int legNode = graph.NodeOf(pointRefs[k - 1]);
            corner.leg = legNode >= 0 && graph.AreaOf(legNode) == AreaWater ? LegKind::Swim : LegKind::Walk;
            corner.node = segmentEnd ? path[last].node : graph.NodeOf(pointRefs[k]);
            if (corner.node < 0)
                corner.node = legNode;
            corners.push_back(corner);
            rcVcopy(previous, point);
        }

        if (link)
        {
            Corner corner;
            rcVcopy(corner.pos, link->toPos);
            corner.leg = LegKind::Link;
            corner.tag = link->tag;
            corner.node = link->to;
            corners.push_back(corner);
            rcVcopy(segmentStart, link->toPos);
            rcVcopy(previous, link->toPos);
        }
        first = last + 1;
    }
    return !corners.empty();
}
} // namespace NavCore
