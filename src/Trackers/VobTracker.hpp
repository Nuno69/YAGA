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
    const char *Name;
    const char *EmptyMessage;
    float MaxDistance;
    bool (*Matches)(zCVob *);
};

inline std::vector<zCVob *> Candidates;
inline zCVob *SelectedVob = nullptr;
inline int CurrentCategoryIndex = 0;

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
    {"All", "No tracked objects", 15000.0f, &MatchesAllCategory},
    {"NPC", "No tracked NPCs", 15000.0f, &MatchesNpcCategory},
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
        Speech::Read(GetCurrentCategory().EmptyMessage);
        return;
    }

    Speech::ReadFormat("{}; {}", GetTrackedVobName(vob).ToChar(), static_cast<int>(GetDistanceToPlayer(vob)));
}

void CycleCategory()
{
    CurrentCategoryIndex = (CurrentCategoryIndex + 1) % static_cast<int>(Categories.size());
    SelectedVob = nullptr;
    RefreshCandidates();
    Speech::Read(GetCurrentCategory().Name);
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
        Speech::Read(GetCurrentCategory().EmptyMessage);
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

    if (zinput->KeyToggled(KEY_LBRACKET))
        ReadSelectedVob(MoveSelection(-1));

    if (zinput->KeyToggled(KEY_RBRACKET))
        ReadSelectedVob(MoveSelection(1));

    if (zinput->KeyToggled(KEY_HOME))
        PointPlayerAtSelectedVob();
}
} // namespace VobTracker

void InitializeVobTracker()
{
    VobTracker::Candidates.clear();
    VobTracker::SelectedVob = nullptr;
    VobTracker::CurrentCategoryIndex = 0;
}

void VobTrackerLoop()
{
    if (!player || !zinput)
        return;

    VobTracker::RefreshCandidates();
    VobTracker::HandleInput();
}
} // namespace GOTHIC_NAMESPACE
