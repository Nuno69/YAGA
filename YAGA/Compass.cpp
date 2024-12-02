// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	float GetCompassAngle() {
		zVEC3 atVectorWorld3D = player->GetAtVectorWorld();
		zVEC2 atVectorWorld2D = zVEC2(atVectorWorld3D[VX], atVectorWorld3D[VZ]);
		return atVectorWorld2D.GetAngle() * DEGREE;
	}

	void RotateTowardsCompassAngle(int direction) {
		float currentAngle = GetCompassAngle();

		int remainder = static_cast<int>(currentAngle) % Settings::zCompassAngleOffset;
		if (direction == 1)
			remainder = Settings::zCompassAngleOffset - remainder;

		float offsetAngle = remainder != 0 ? remainder * direction : Settings::zCompassAngleOffset * direction;
		player->RotateLocalY(offsetAngle);
	}
}