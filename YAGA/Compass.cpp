// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	int GetCompassAngle() {
		zVEC3 atVectorWorld3D = player->GetAtVectorWorld();
		zVEC2 atVectorWorld2D = zVEC2( atVectorWorld3D[VX], atVectorWorld3D[VZ] );
		int atVectorAngle = (int)( atVectorWorld2D.GetAngle() * DEGREE );
		return atVectorAngle;
	}
}