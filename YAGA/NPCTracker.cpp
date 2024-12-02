// Supported with union (c) 2020 Union team
// Union SOURCE file
#include <algorithm>
namespace GOTHIC_ENGINE {
	// Add your code here . . .
	int NPCTracker::MoveToPreviousNPC()
	{
		cursor--;
		if (nearbyNPC.size() == 0) return 0;
		if (cursor < 0) cursor = numberOfNPC - 1;
		return 1;
	}

	int NPCTracker::MoveToNextNPC()
	{
		cursor++;
		if (nearbyNPC.size() == 0) return 0;
		if (cursor >= numberOfNPC) cursor = 0;
		return 1;
	}

	int NPCTracker::SelectNPCAtCursor()
	{
		if (nearbyNPC.size() == 0 || !nearbyNPC[cursor]) return 0;
		selectedNPC = nearbyNPC[cursor];
		return 1;
	}

	int NPCTracker::AddNPC(oCNpc* npc)
	{
		if (!npc) return 0;
		nearbyNPC.push_back(npc);
		UpdateNPCNumber();
		return 1;
	}

	int NPCTracker::GetCurrentNumOfNPC() { return numberOfNPC; }

	void NPCTracker::ClearList() { nearbyNPC.clear(); }

	void NPCTracker::UpdateNPCNumber() { numberOfNPC = nearbyNPC.size(); }

	void NPCTracker::SortNPCByDistanceFromPlayer()
	{
		std::sort(nearbyNPC.begin(), nearbyNPC.end(), [](oCNpc* lh, oCNpc* rh) {return (lh->GetDistanceToPos2(ogame->GetSelfPlayerVob()->GetPositionWorld(), 0) < rh->GetDistanceToPos2(ogame->GetSelfPlayerVob()->GetPositionWorld(), 0)); });
	}
	oCNpc* NPCTracker::GetSelectedNPC()
	{
		if (selectedNPC) return selectedNPC;
		return nullptr;
	}
}