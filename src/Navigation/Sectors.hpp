#ifndef zMAP_SECTOR_SIZE
#define zMAP_SECTOR_SIZE 5000
#endif

namespace GOTHIC_NAMESPACE
{
using SECTOR = POINT;

// Floor division, so every sector is exactly zMAP_SECTOR_SIZE wide. Truncating (int(x / size)) made sector 0
// twice as wide as the others because it swallowed the coordinates -4999..4999.
inline LONG SectorCoord(const float worldCoord)
{
    return static_cast<LONG>(std::floor(worldCoord / static_cast<float>(zMAP_SECTOR_SIZE)));
}

inline SECTOR SectorOf(const zVEC3 &position)
{
    SECTOR sector;
    sector.x = SectorCoord(position[VX]);
    sector.y = SectorCoord(position[VZ]);
    return sector;
}

inline bool SameSector(const SECTOR &a, const SECTOR &b)
{
    return a.x == b.x && a.y == b.y;
}

inline zVEC3 SectorCenter(const SECTOR &sector, const float height = 0.0f)
{
    return zVEC3((static_cast<float>(sector.x) + 0.5f) * zMAP_SECTOR_SIZE, height,
                 (static_cast<float>(sector.y) + 0.5f) * zMAP_SECTOR_SIZE);
}

inline float DistanceXZ(const zVEC3 &a, const zVEC3 &b)
{
    const float dx = a[VX] - b[VX];
    const float dz = a[VZ] - b[VZ];
    return std::sqrt(dx * dx + dz * dz);
}
} // namespace GOTHIC_NAMESPACE
