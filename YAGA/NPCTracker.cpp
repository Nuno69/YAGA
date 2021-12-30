// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Add your code here . . .
int NPCTracker::MoveToPreviousNPC()
{
cursor --;
		if (nearbyNPC.GetNum() == 0) return 0;
if (cursor < 0) cursor = numberOfNPC -1;
return 1;
}

int NPCTracker::MoveToNextNPC()
{
	cursor++;
	if (nearbyNPC.GetNum() == 0) return 0;
	if (cursor >= numberOfNPC) cursor = 0;
return 1;
}

int NPCTracker::SelectNPCAtCursor()
{
if (nearbyNPC.GetNum() == 0 || !nearbyNPC[cursor]) return 0;
selectedNPC = nearbyNPC[cursor];
return 1;
}

int NPCTracker::AddNPC(oCNpc *npc)
{
if (!npc) return 0;
nearbyNPC.InsertEnd(npc);
return 1;
}

void NPCTracker::ClearList() {nearbyNPC.EmptyList();}

void NPCTracker::UpdateNPCNumber() {numberOfNPC = nearbyNPC.GetNum(); }
}