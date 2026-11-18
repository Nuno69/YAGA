#include <vector>

namespace GOTHIC_NAMESPACE
{
// Named places of a world (Places_G1.hpp): what the sector map says on ENTER and where SPACE leads when a sector
// holds one. Each place is a waypoint (way net or patch node); its sector follows from the waypoint position.
namespace Places
{
struct Place
{
    const wchar_t *English;
    const wchar_t *Polish;
    const char *Waypoint;
};

// Defined per game in Places_G1.hpp.
inline const std::vector<Place> &Table();
inline const char *PlacesWorld();

struct Entry
{
    const Place *Info = nullptr;
    zCWaypoint *Waypoint = nullptr;
    SECTOR Sector{};
};

inline std::vector<Entry> Entries;
inline zCWayNet *ResolvedFor = nullptr;
inline int ResolvedGeneration = -1;

inline std::wstring Name(const Entry &entry)
{
    return Loc::Polish ? entry.Info->Polish : entry.Info->English;
}

// Looks the waypoints up once per way net (and again after the patch is rebuilt, which replaces its nodes).
inline void EnsureResolved()
{
    oCWorld *world = ogame ? ogame->GetGameWorld() : nullptr;
    zCWayNet *wayNet = world ? world->wayNet : nullptr;
    if (!wayNet)
    {
        Entries.clear();
        ResolvedFor = nullptr;
        return;
    }

    WaynetPatch::EnsureBuilt();
    if (wayNet == ResolvedFor && ResolvedGeneration == WaynetPatch::Generation)
        return;

    Entries.clear();
    ResolvedFor = wayNet;
    ResolvedGeneration = WaynetPatch::Generation;
    if (WaynetPatch::BuiltWorld != PlacesWorld())
        return;

    for (const Place &place : Table())
    {
        zCWaypoint *wp = WaynetPatch::Find(wayNet, place.Waypoint);
        if (!wp)
        {
            YagaLog::Write(std::string("[places] missing waypoint ") + place.Waypoint);
            continue;
        }
        Entries.push_back({&place, wp, SectorOf(wp->GetPositionWorld())});
    }
}

// Places in a sector, the one nearest the sector centre first.
inline std::vector<const Entry *> InSector(const SECTOR &sector)
{
    EnsureResolved();
    std::vector<const Entry *> result;
    for (const Entry &entry : Entries)
        if (SameSector(entry.Sector, sector))
            result.push_back(&entry);

    const zVEC3 centre = SectorCenter(sector);
    std::sort(result.begin(), result.end(), [&](const Entry *a, const Entry *b) {
        return DistanceXZ(a->Waypoint->GetPositionWorld(), centre) < DistanceXZ(b->Waypoint->GetPositionWorld(), centre);
    });
    return result;
}

// Every place, alphabetically in the current language.
inline std::vector<const Entry *> Sorted()
{
    EnsureResolved();
    std::vector<const Entry *> result;
    for (const Entry &entry : Entries)
        result.push_back(&entry);
    std::sort(result.begin(), result.end(), [](const Entry *a, const Entry *b) {
        return CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, Name(*a).c_str(), -1, Name(*b).c_str(), -1) ==
               CSTR_LESS_THAN;
    });
    return result;
}

// "Stary Obóz, Zamek" for a sector with places, empty without.
inline std::wstring NamesInSector(const SECTOR &sector)
{
    std::wstring names;
    for (const Entry *entry : InSector(sector))
    {
        if (!names.empty())
            names += L", ";
        names += Name(*entry);
    }
    return names;
}
} // namespace Places
} // namespace GOTHIC_NAMESPACE
