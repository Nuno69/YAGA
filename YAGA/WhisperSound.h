// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	void UpdateSoundHandles();
	bool BindSound3D(const string& fileName, zCVob* vob);
	bool UnbindSound3D(const string& fileName, zCVob* vob);
}