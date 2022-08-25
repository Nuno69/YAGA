// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Add your code here . . .
	static zCVob* LastFocusedVob = nullptr;
	static oCItem* item = nullptr;
	static oCNpc* npc = nullptr;
	void FocusReaderLoop() {
		zCVob* currentVob = player->GetFocusVob();
		if (currentVob == nullptr) { return; }
			LastFocusedVob = currentVob;
			// Maybe the vob is an item? Or an NPC? We'll see!
			if (currentVob != LastFocusedVob) {
				item = currentVob->CastTo<oCItem>();
				npc = currentVob->CastTo<oCNpc>();
				if (item != nullptr) {
					Read(string(item->name).AToW());
					return;
				}
				if (npc != nullptr) {
					Read(string(npc->GetName(0)).AToW());
					return;
				}
				else {}
				Read(string(currentVob->GetObjectName()).AToW());
				return;
			}
		}
}