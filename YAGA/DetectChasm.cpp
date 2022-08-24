// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	HOOK Hook_oCNpc_EV_Strafe PATCH( &oCNpc::EV_Strafe, &oCNpc::EV_Strafe_Union );

	int oCNpc::EV_Strafe_Union( oCMsgMovement* msg ) {
		int ok = THISCALL( Hook_oCNpc_EV_Strafe )(msg);
		if( this == player && anictrl->walkmode == ANI_WALKMODE_RUN )
			SetWalkStopChasm( !ok );

		return ok;
	}
}