#ifndef YAGA_NPC_TRACKER_SETTINGS
#define YAGA_NPC_TRACKER_SETTINGS

namespace Settings
{
inline int NPCTrackerReadIndex;
inline int NPCTrackerReadAngle;
} // namespace Settings

#endif

#ifdef GOTHIC_NAMESPACE

namespace GOTHIC_NAMESPACE
{
void LoadNPCTrackerSettings()
{
    Settings::NPCTrackerReadIndex = zoptions->ReadInt("YAGA", "NPCTrackerReadIndex", 1);
    Settings::NPCTrackerReadAngle = zoptions->ReadInt("YAGA", "NPCTrackerReadAngle", 1);
}

class NPCTracker
{
  private:
    oCNpc *selectedNPC;
    std::vector<oCNpc *> nearbyNPC;
    int cursor = 0;
    int numberOfNPC = 0;

  public:
    int MoveToPreviousNPC()
    {
        cursor--;
        if (nearbyNPC.size() == 0)
            return 0;
        if (cursor < 0)
            cursor = numberOfNPC - 1;
        return 1;
    }

    int MoveToNextNPC()
    {
        cursor++;
        if (nearbyNPC.size() == 0)
            return 0;
        if (cursor >= numberOfNPC)
            cursor = 0;
        return 1;
    }

    int SelectNPCAtCursor()
    {
        if (nearbyNPC.size() == 0 || !nearbyNPC[cursor])
            return 0;
        selectedNPC = nearbyNPC[cursor];
        return 1;
    }

    int AddNPC(oCNpc *npc)
    {
        if (!npc)
            return 0;
        nearbyNPC.push_back(npc);
        UpdateNPCNumber();
        return 1;
    }

    int GetCurrentNumOfNPC()
    {
        return numberOfNPC;
    }

    void ClearList()
    {
        nearbyNPC.clear();
    }

    void UpdateNPCNumber()
    {
        numberOfNPC = nearbyNPC.size();
    }

    void SortNPCByDistanceFromPlayer()
    {
        zVEC3 playerPosition = ogame->GetSelfPlayerVob()->GetPositionWorld();
        std::sort(nearbyNPC.begin(), nearbyNPC.end(), [&playerPosition](oCNpc *lh, oCNpc *rh) {
            return (lh->GetDistanceToPos2(playerPosition, 0) < rh->GetDistanceToPos2(playerPosition, 0));
        });
    }

    oCNpc *GetSelectedNPC()
    {
        if (selectedNPC)
            return selectedNPC;
        return nullptr;
    }

  public:
    ~NPCTracker()
    {
        nearbyNPC.clear();
    }
};
} // namespace GOTHIC_NAMESPACE

#endif