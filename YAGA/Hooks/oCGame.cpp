// Supported with union (c)2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
#if ENGINE <= Engine_G1A
	// One time hook that enables Hook_oCWorld_CreateVob.
	HOOK Hook_oCGame_SetupPlayers AS(&oCGame::SetupPlayers, &oCGame::Hook_SetupPlayers);
	void oCGame::Hook_SetupPlayers(oCNpc*& player, const zSTRING& startPoint)
	{
		Hook_oCGame_SetupPlayers.Detach();
		Hook_oCWorld_CreateVob.Commit();

		THISCALL(Hook_oCGame_SetupPlayers)(player, startPoint);
	}
#endif
}