// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Add your code here . . .
	static zCVob* LastFocusedVob = nullptr;
	void FocusReaderLoop() {
		zCVob* currentVob = player->GetFocusVob();
		if (currentVob == nullptr) { return; }
		if (currentVob != LastFocusedVob) {
			LastFocusedVob = currentVob;
			Read(string(currentVob->GetObjectName()).AToW());
		}
}
}