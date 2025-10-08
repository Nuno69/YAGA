namespace GOTHIC_NAMESPACE {
	float GetCompassAngle()
	{
		zVEC3 atVectorWorld3D = player->GetAtVectorWorld();
		zVEC2 atVectorWorld2D = zVEC2(atVectorWorld3D[VX], atVectorWorld3D[VZ]);
		return atVectorWorld2D.GetAngle() * DEGREE;
	}

	void RotateTowardsCompassAngle(int direction)
	{
		const float currentAngle = GetCompassAngle();

		int remainder = static_cast<int>(currentAngle) % Settings::CompassAngleOffset;
		if (direction == 1)
			remainder = Settings::CompassAngleOffset - remainder;

		const float offsetAngle = remainder != 0 ? static_cast<float>(remainder * direction) : Settings::CompassAngleOffset * direction;
		player->RotateLocalY(offsetAngle);
	}
}