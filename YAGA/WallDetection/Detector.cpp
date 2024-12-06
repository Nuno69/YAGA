// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	void zDetector::DetectNpcs() {
		auto npcsList = ogame->GetGameWorld()->voblist_npcs->next;
		while (npcsList) {
			oCNpc* npc = npcsList->data;
			{

			}
			npcsList = npcsList->next;
		}
	}
}