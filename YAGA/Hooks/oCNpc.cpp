// Supported with union (c)2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Prevents the playable character from falling from ledges.
	HOOK Hook_oCNpc_SetWalkStopChasm AS(&oCNpc::SetWalkStopChasm, &oCNpc::Hook_SetWalkStopChasm);
	void oCNpc::Hook_SetWalkStopChasm(int stop)
	{
		THISCALL(Hook_oCNpc_SetWalkStopChasm)(this == player ? TRUE : stop);
	}

	// Removes the line of sight obstruction.
	HOOK Patch_oCNpc_FreeLineOfSight PATCH(&oCNpc::FreeLineOfSight, &oCNpc::Patch_FreeLineOfSight);
	int oCNpc::Patch_FreeLineOfSight(zVEC3& pos, zCVob* vob)
	{
		return TRUE;
	}

	// Disables NPC removal from world, will be used in navigation.
	HOOK Hook_oCNpc_Disable_Union AS(&oCNpc::Disable, &oCNpc::Hook_Disable);
	void oCNpc::Hook_Disable()
	{
		zVEC3 routPosition = state.GetAIStatePosition();
		if (routPosition != 0.0f)
			SetPositionWorld(routPosition);

		THISCALL(Hook_oCNpc_Disable_Union)();
	}
}