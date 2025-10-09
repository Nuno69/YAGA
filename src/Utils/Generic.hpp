namespace GOTHIC_NAMESPACE {
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