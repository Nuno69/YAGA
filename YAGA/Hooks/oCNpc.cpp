// Supported with union (c)2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Prevents the playable character from falling from ledges.
	HOOK Hook_oCNpc_EV_Strafe PATCH(&oCNpc::EV_Strafe, &oCNpc::EV_Strafe_Union);
	int oCNpc::EV_Strafe_Union(oCMsgMovement* msg)
	{
		int ok = THISCALL(Hook_oCNpc_EV_Strafe)(msg);
		if (this == player && anictrl->walkmode == ANI_WALKMODE_RUN)
			SetWalkStopChasm(!ok);

		return ok;
	}

	// Removes the line of sight obstruction.
	HOOK Hook_oCNpc_FreeLineOfSight PATCH(&oCNpc::FreeLineOfSight, &oCNpc::FreeLineOfSight_Union);
	int oCNpc::FreeLineOfSight_Union(zVEC3& pos, zCVob* vob)
	{
		return TRUE;
	}

	// Disables NPC removal from world, will be used in navigation.
	HOOK Hook_oCNpc_Disable_Union PATCH(&oCNpc::Disable, &oCNpc::Disable_Union);
	void oCNpc::Disable_Union()
	{
		zVEC3 routPosition = state.GetAIStatePosition();
		if (routPosition != 0.0f)
			SetPositionWorld(routPosition);

		THISCALL(Hook_oCNpc_Disable_Union)();
	}
}