// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	void BlindCameraLoop() {
		zCVob* cameraVob = ogame->GetCameraVob();
		cameraVob->SetSleeping( True );
		zVEC3 playerHeadPosition = player->GetPositionWorld() + zVEC3( 0.0f, 70.0f, 0.0f );
		zVEC3 playerAtVector = player->GetAtVectorWorld();
		cameraVob->SetPositionWorld( playerHeadPosition );
		cameraVob->SetHeadingYWorld( playerHeadPosition + playerAtVector );
	}
}