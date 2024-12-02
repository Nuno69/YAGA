// Supported with union (c) 2020 Union team
// Union HEADER file
#include <vector>
namespace GOTHIC_ENGINE {
	// Add your code here . . .
	namespace Settings
	{
		int NPCTrackerReadIndex, NPCTrackerReadAngle;
		void NPCTracker()
		{
			NPCTrackerReadIndex = zoptions->ReadInt("YAGA", "NPCTrackerReadIndex", 1);
			NPCTrackerReadAngle = zoptions->ReadInt("YAGA", "NPCTrackerReadAngle", 1);
		}
	}
	class NPCTracker
	{
	private:
		oCNpc* selectedNPC;
		std::vector<oCNpc*> nearbyNPC;
		int cursor = 0;
		int numberOfNPC = 0;

	public:
		int MoveToPreviousNPC();
		int MoveToNextNPC();
		int SelectNPCAtCursor();
		int AddNPC(oCNpc* npc);
		int GetCurrentNumOfNPC();
		void ClearList();
		void UpdateNPCNumber();
		void SortNPCByDistanceFromPlayer();
		oCNpc* GetSelectedNPC();

	public:
		~NPCTracker() { nearbyNPC.clear(); }
	};
}