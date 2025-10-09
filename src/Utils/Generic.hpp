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

	float GetAngleRelation(zCVob* source, zCVob* target)
	{

		zVEC3 position = (source->GetPositionWorld() - target->GetPositionWorld().Normalize());
		zVEC3 at = source->GetAtVectorWorld();
		float radian = Alg_AngleUnitRad(position, at);
		return radian * DEGREE;
	}

	bool IsBehind(float angle)
	{
		if (angle >= 0 || angle <= 90 || angle >= 270 || angle <= 359)
			return false;

		return true;
	}
}