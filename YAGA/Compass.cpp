// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	const int compassAngleOffset = 45;

	float GetCompassAngle() {
		zVEC3 atVectorWorld3D = player->GetAtVectorWorld();
		zVEC2 atVectorWorld2D = zVEC2( atVectorWorld3D[VX], atVectorWorld3D[VZ] );
		return atVectorWorld2D.GetAngle() * DEGREE;
	}

	void RotateTowardsCompassAngle(int direction) {
		float currentAngle = GetCompassAngle();

		int remainder = static_cast<int>(currentAngle) % compassAngleOffset;
		if (direction == 1)
			remainder = compassAngleOffset - remainder;

		float offsetAngle = remainder != 0 ? remainder * direction : compassAngleOffset * direction;
		player->RotateLocalY(offsetAngle);
	}
}