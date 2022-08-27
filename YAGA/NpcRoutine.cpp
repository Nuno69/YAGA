// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	HOOK Hook_oCNpc_Disable_Union PATCH( &oCNpc::Disable, &oCNpc::Disable_Union );

	void oCNpc::Disable_Union() {
		zVEC3 routPosition = state.GetAIStatePosition();
		if( routPosition != 0.0f )
			SetPositionWorld( routPosition );

		THISCALL( Hook_oCNpc_Disable_Union )();
	}
}