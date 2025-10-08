namespace GOTHIC_NAMESPACE {
	// Prevents the playable character from falling from ledges.
	auto Hook_oCNpc_SetWalkStopChasm = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x00688A70, 0x00688A70, 0x00688A70, 0x00688A70)), &oCNpc::Hook_SetWalkStopChasm, Union::HookType::Hook_Detours);
	void oCNpc::Hook_SetWalkStopChasm(int stop)
	{
		(this->*Hook_oCNpc_SetWalkStopChasm)(this == player ? TRUE : stop);
	}

	// Removes the line of sight obstruction.
	auto Patch_oCNpc_FreeLineOfSight = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x007418E0, 0x007418E0, 0x007418E0, 0x007418E0)), &oCNpc::Patch_FreeLineOfSight, Union::HookType::Hook_CallPatch);
	int oCNpc::Patch_FreeLineOfSight(zVEC3& pos, zCVob* vob)
	{
		return TRUE;
	}

	// Disables NPC removal from world, will be used in navigation.
	auto Hook_oCNpc_Disable_Union = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x00745A20, 0x00745A20, 0x00745A20, 0x00745A20)), &oCNpc::Hook_Disable, Union::HookType::Hook_Detours);
	void oCNpc::Hook_Disable()
	{
		zVEC3 routPosition = state.GetAIStatePosition();
		if (routPosition != 0.0f)
			SetPositionWorld(routPosition);

		(this->*Hook_oCNpc_Disable_Union)();
	}
}