#include <algorithm>
#include <array>
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
    return zDYNAMIC_CAST<oCNpc>(vob) != nullptr && vob != player;
}

const std::array<Category, 2> Categories = {{
    {{L"All", L"Wszystko"}, {L"No tracked objects", L"Brak obiektów w pobliżu"}, 15000.0f, &MatchesAllCategory},
    {{L"NPC", L"Postacie"}, {L"No tracked NPCs", L"Brak postaci w pobliżu"}, 15000.0f, &MatchesNpcCategory},
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

void ReadSelectedVob(zCVob *vob)
{
    if (!vob)
    {
        Speech::Read(GetCurrentCategory().EmptyMessage[Loc::Polish ? 1 : 0]);
        return;
    }

    Speech::ReadFormat("{}; {}", GetTrackedVobName(vob).ToChar(), static_cast<int>(GetDistanceToPlayer(vob)));
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
