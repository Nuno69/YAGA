#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

namespace GOTHIC_NAMESPACE
{
namespace VobTracker
{
struct Category
{
    const wchar_t *Name[2];         // English, Polish
    const wchar_t *EmptyMessage[2]; // English, Polish
    float MaxDistance;
    bool (*Matches)(zCVob *);
};

inline std::vector<zCVob *> Candidates;
inline zCVob *SelectedVob = nullptr;
inline int CurrentCategoryIndex = 0;
inline bool RadarEnabled = false;

bool IsSupportedVob(zCVob *vob)
{
    if (!vob || vob == player)
        return false;

    return zDYNAMIC_CAST<oCNpc>(vob) || zDYNAMIC_CAST<oCItem>(vob) || zDYNAMIC_CAST<oCMobInter>(vob);
}

bool MatchesAllCategory(zCVob *vob)
{
    return IsSupportedVob(vob);
}

bool MatchesNpcCategory(zCVob *vob)
{
    oCNpc *npc = zDYNAMIC_CAST<oCNpc>(vob);
    return npc && vob != player && npc->IsHuman();
}

// Monsters, and orcs: every character that is not human.
bool MatchesMonsterCategory(zCVob *vob)
{
    oCNpc *npc = zDYNAMIC_CAST<oCNpc>(vob);
    return npc && vob != player && !npc->IsHuman();
}

bool MatchesItemCategory(zCVob *vob)
{
    return zDYNAMIC_CAST<oCItem>(vob) != nullptr;
}

// Things to use: chests, beds, fires, doors, ladders, switches.
bool MatchesObjectCategory(zCVob *vob)
{
    return zDYNAMIC_CAST<oCMobInter>(vob) != nullptr;
}

const std::array<Category, 5> Categories = {{
    {{L"All", L"Wszystko"}, {L"No tracked objects", L"Brak obiektów w pobliżu"}, 15000.0f, &MatchesAllCategory},
    {{L"Characters", L"Postacie"}, {L"No characters nearby", L"Brak postaci w pobliżu"}, 15000.0f,
     &MatchesNpcCategory},
    {{L"Monsters", L"Potwory"}, {L"No monsters nearby", L"Brak potworów w pobliżu"}, 15000.0f,
     &MatchesMonsterCategory},
    {{L"Items", L"Przedmioty"}, {L"No items nearby", L"Brak przedmiotów w pobliżu"}, 15000.0f, &MatchesItemCategory},
    {{L"Objects", L"Obiekty"}, {L"No objects nearby", L"Brak obiektów do użycia w pobliżu"}, 15000.0f,
     &MatchesObjectCategory},
}};

const Category &GetCurrentCategory()
{
    return Categories[CurrentCategoryIndex];
}

zSTRING GetTrackedVobName(zCVob *vob)
{
    if (!vob)
        return "Unknown";

    if (oCNpc *npc = zDYNAMIC_CAST<oCNpc>(vob))
        return npc->GetName(0);

    if (oCItem *item = zDYNAMIC_CAST<oCItem>(vob))
        return item->name;

    if (oCMobInter *mob = zDYNAMIC_CAST<oCMobInter>(vob))
    {
        const zSTRING mobName = mob->GetName();
        if (!mobName.IsEmpty())
            return mobName;
    }

    const zSTRING objectName = vob->GetObjectName();
    if (!objectName.IsEmpty())
        return objectName;

    return "Unknown";
}

float GetDistanceToPlayer(zCVob *vob)
{
    if (!player || !vob)
        return 0.0f;

    return player->GetPositionWorld().Distance(vob->GetPositionWorld());
}

SpatialCues::DistancePulseSettings GetTrackedCuePulseSettings()
{
    SpatialCues::DistancePulseSettings settings;
    settings.NearIntervalMs = 250.0f;
    settings.FarIntervalMs = 2000.0f;
    settings.MaxDistance = 15000.0f;
    return settings;
}

zCSoundSystem::zTSound3DParams MakeTrackedCue3DProps()
{
    zCSoundSystem::zTSound3DParams props;
    props.SetDefaults();
    props.loopType = zCSoundSystem::zSND_LOOPING_DISABLED;
    props.radius = 15000.0f;
    props.volume = 1.0f;
    props.reverbLevel = 1.0f;
    props.pitchOffset = 0.0f;
    return props;
}

void RefreshCandidates()
{
    zCVob *previousSelection = SelectedVob;
    Candidates.clear();

    oCWorld *world = ogame ? ogame->GetGameWorld() : nullptr;
    if (!player || !world)
    {
        SelectedVob = nullptr;
        return;
    }

    const Category &category = GetCurrentCategory();

    for (zCListSort<zCVob> *node = world->voblist; node; node = node->next)
    {
        zCVob *vob = node->data;
        if (!vob || !category.Matches(vob))
            continue;

        if (GetDistanceToPlayer(vob) > category.MaxDistance)
            continue;

        Candidates.push_back(vob);
    }

    std::sort(Candidates.begin(), Candidates.end(), [](zCVob *left, zCVob *right) {
        const float leftDistance = GetDistanceToPlayer(left);
        const float rightDistance = GetDistanceToPlayer(right);
        if (leftDistance != rightDistance)
            return leftDistance < rightDistance;

        const zSTRING leftName = GetTrackedVobName(left);
        const zSTRING rightName = GetTrackedVobName(right);
        return std::strcmp(leftName.ToChar(), rightName.ToChar()) < 0;
    });

    if (previousSelection &&
        std::find(Candidates.begin(), Candidates.end(), previousSelection) != Candidates.end())
    {
        SelectedVob = previousSelection;
        return;
    }

    SelectedVob = nullptr;
}

zCVob *GetSelectedVob()
{
    if (!SelectedVob)
        return nullptr;

    const auto it = std::find(Candidates.begin(), Candidates.end(), SelectedVob);
    if (it == Candidates.end())
    {
        SelectedVob = nullptr;
        return nullptr;
    }

    return SelectedVob;
}

// Where a point is as seen by the hero: ahead, ahead right, right ... ahead left (eight directions).
std::wstring DirectionFromHero(const zVEC3 &target)
{
    const zVEC3 hero = player->GetPositionWorld();
    zVEC3 at = player->GetAtVectorWorld();
    zVEC3 right = player->GetRightVectorWorld();
    const float dx = target[VX] - hero[VX];
    const float dz = target[VZ] - hero[VZ];
    const float forward = dx * at[VX] + dz * at[VZ];
    const float side = dx * right[VX] + dz * right[VZ];
    const float angle = std::atan2(side, forward) * 180.0f / 3.14159265f; // 0 ahead, 90 right
    const int sector = (static_cast<int>(std::lround(angle / 45.0f)) + 8) % 8;
    static const wchar_t *english[8] = {L"ahead",  L"ahead right", L"right", L"behind right",
                                        L"behind", L"behind left", L"left",  L"ahead left"};
    static const wchar_t *polish[8] = {L"z przodu", L"z przodu po prawej", L"po prawej", L"z tyłu po prawej",
                                       L"z tyłu",   L"z tyłu po lewej",    L"po lewej",  L"z przodu po lewej"};
    return Loc::T(english[sector], polish[sector]);
}

// Whether a character is hostile to the hero.
bool IsHostile(zCVob *vob)
{
    oCNpc *npc = zDYNAMIC_CAST<oCNpc>(vob);
    return npc && player && !npc->IsDead() && npc->GetAttitude(player) == NPC_ATT_HOSTILE;
}

// How close the navigator walks the hero to an object: near enough to talk to a person, pick up an item or use a
// chest; a monster is approached only to a few metres.
float StopDistanceFor(zCVob *vob)
{
    if (oCNpc *npc = zDYNAMIC_CAST<oCNpc>(vob))
        return npc->IsHuman() || npc->IsDead() ? 200.0f : 500.0f;
    if (zDYNAMIC_CAST<oCItem>(vob))
        return 120.0f;
    if (zDYNAMIC_CAST<oCMobInter>(vob))
        return 150.0f;
    return 200.0f;
}

// The selected object: its name, then the distance in metres, the direction from the hero, how much higher or lower
// it is, whether the hero can walk there (and how far the walk is when it is much longer than the straight line),
// and whether a character is dead or hostile.
void ReadSelectedVob(zCVob *vob)
{
    if (!vob)
    {
        Speech::Read(GetCurrentCategory().EmptyMessage[Loc::Polish ? 1 : 0]);
        return;
    }

    const zVEC3 hero = player->GetPositionWorld();
    const zVEC3 target = vob->GetPositionWorld();
    const int meters = static_cast<int>(GetDistanceToPlayer(vob) / 100.0f + 0.5f);
    std::wstring text = ToWString(GetTrackedVobName(vob)) + L", " + Loc::Meters(meters) + L", " +
                        DirectionFromHero(target);

    const int rise = static_cast<int>((target[VY] - hero[VY]) / 100.0f);
    if (rise >= 2)
        text += Loc::Format(L", {} higher", L", {} wyżej", Loc::Meters(rise));
    else if (rise <= -2)
        text += Loc::Format(L", {} lower", L", {} niżej", Loc::Meters(-rise));

    float walk = 0.0f;
    switch (NavMesh::RouteLength(hero, target, walk))
    {
    case NavMesh::Reach::Unreachable:
        text += Loc::T(L", unreachable", L", nieosiągalne");
        break;
    case NavMesh::Reach::Reachable:
        if (walk > GetDistanceToPlayer(vob) * 1.3f + 500.0f)
            text += Loc::Format(L", {} to walk", L", drogą {}", Loc::Meters(static_cast<int>(walk / 100.0f + 0.5f)));
        break;
    default:
        break;
    }

    if (oCNpc *npc = zDYNAMIC_CAST<oCNpc>(vob))
    {
        if (npc->IsDead())
            text += Loc::T(L", dead", L", martwy");
        else if (IsHostile(vob))
            text += Loc::T(L", hostile", L", wrogi");
    }
    Speech::Read(text);
}

void CycleCategory()
{
    CurrentCategoryIndex = (CurrentCategoryIndex + 1) % static_cast<int>(Categories.size());
    SelectedVob = nullptr;
    RefreshCandidates();
    Speech::Read(GetCurrentCategory().Name[Loc::Polish ? 1 : 0]);
}

void ToggleRadar()
{
    RadarEnabled = !RadarEnabled;
    Speech::Read(RadarEnabled ? Loc::T(L"Enabled tracking", L"Śledzenie włączone")
                              : Loc::T(L"Disabled tracking", L"Śledzenie wyłączone"));
}

zCVob *MoveSelection(const int direction)
{
    if (Candidates.empty())
    {
        SelectedVob = nullptr;
        return nullptr;
    }

    zCVob *currentSelection = GetSelectedVob();
    if (!currentSelection)
    {
        SelectedVob = direction < 0 ? Candidates.back() : Candidates.front();
        return SelectedVob;
    }

    int currentIndex = static_cast<int>(std::distance(
        Candidates.begin(), std::find(Candidates.begin(), Candidates.end(), currentSelection)));
    currentIndex += direction;

    if (currentIndex < 0)
        currentIndex = static_cast<int>(Candidates.size()) - 1;
    else if (currentIndex >= static_cast<int>(Candidates.size()))
        currentIndex = 0;

    SelectedVob = Candidates[currentIndex];
    return SelectedVob;
}

void PointPlayerAtSelectedVob()
{
    zCVob *trackedVob = GetSelectedVob();
    if (!trackedVob)
    {
        Speech::Read(GetCurrentCategory().EmptyMessage[Loc::Polish ? 1 : 0]);
        return;
    }

    zVEC3 targetPosition = trackedVob->GetPositionWorld();
    targetPosition[VY] = player->GetPositionWorld()[VY];
    player->SetHeadingYWorld(targetPosition);
}

void HandleInput()
{
    if (zinput->KeyToggled(KEY_T))
        CycleCategory();

    if (zinput->KeyToggled(KEY_R))
        ToggleRadar();

    if (zinput->KeyToggled(KEY_LBRACKET))
        ReadSelectedVob(MoveSelection(-1));

    if (zinput->KeyToggled(KEY_RBRACKET))
        ReadSelectedVob(MoveSelection(1));

    if (zinput->KeyToggled(KEY_HOME))
        PointPlayerAtSelectedVob();
}

void SubmitRadarCue()
{
    if (!RadarEnabled)
        return;

    zCVob *trackedVob = GetSelectedVob();
    if (!trackedVob)
        return;

    SpatialCues::SubmitVobCue(
        {trackedVob, "tracked"},
        "TRACKED.WAV",
        trackedVob,
        MakeTrackedCue3DProps(),
        GetTrackedCuePulseSettings());
}
} // namespace VobTracker

void InitializeVobTracker()
{
    VobTracker::Candidates.clear();
    VobTracker::SelectedVob = nullptr;
    VobTracker::CurrentCategoryIndex = 0;
    VobTracker::RadarEnabled = false;
}

void VobTrackerLoop()
{
    if (!player || !zinput)
        return;

    VobTracker::RefreshCandidates();
    VobTracker::HandleInput();
    VobTracker::SubmitRadarCue();
}
} // namespace GOTHIC_NAMESPACE
