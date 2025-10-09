namespace GOTHIC_NAMESPACE {
	std::string GetCompassName(float _angle)
	{
		int angle = static_cast<int>(_angle - 22);

		if (angle < 45) return std::format("North at {}", angle);
		if (angle < 90) return std::format("North-east at {}", angle);
		if (angle < 135) return std::format("East at {}", angle);
		if (angle < 180) return std::format("South-east at {}", angle);
		if (angle < 225) return std::format("South at {}", angle);
		if (angle < 270) return std::format("South-west at {}", angle);
		if (angle < 315) return std::format("West at {}", angle);
		if (angle < 360) return std::format("North-west at {}", angle);
		return std::format("North at {}", angle);
	}

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