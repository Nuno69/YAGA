namespace GOTHIC_NAMESPACE {
#if ENGINE <= Engine_G1A
	// One time hook that enables Hook_oCWorld_CreateVob.
	auto Hook_oCGame_SetupPlayers = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x006C3FA0, 0x006C3FA0, 0x006C3FA0, 0x006C3FA0)), &oCGame::Hook_SetupPlayers, Union::HookType::Hook_Detours);
	void oCGame::Hook_SetupPlayers(oCNpc*& player, const zSTRING& startPoint)
	{
		Hook_oCGame_SetupPlayers.Detach();
		Hook_oCWorld_CreateVob.Commit();

		(this->*Hook_oCGame_SetupPlayers)(player, startPoint);
	}
#endif
}