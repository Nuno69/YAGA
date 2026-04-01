namespace GOTHIC_NAMESPACE
{
float NormalizeCompassAngle(float angle)
{
    angle = std::fmod(angle, 360.0f);
    if (angle < 0.0f)
        angle += 360.0f;

    return angle;
}

std::string GetCompassName(float _angle)
{
    const int angle = static_cast<int>(NormalizeCompassAngle(_angle));
    const int sector = static_cast<int>((angle + 22) / 45) % 8;

    switch (sector)
    {
    case 0:
        return std::format("North at {}", angle);
    case 1:
        return std::format("North-east at {}", angle);
    case 2:
        return std::format("East at {}", angle);
    case 3:
        return std::format("South-east at {}", angle);
    case 4:
        return std::format("South at {}", angle);
    case 5:
        return std::format("South-west at {}", angle);
    case 6:
        return std::format("West at {}", angle);
    default:
        return std::format("North-west at {}", angle);
    }
}

float GetCompassAngle()
{
    zVEC3 atVectorWorld3D = player->GetAtVectorWorld();
    zVEC2 atVectorWorld2D = zVEC2(atVectorWorld3D[VX], atVectorWorld3D[VZ]);
    return NormalizeCompassAngle(atVectorWorld2D.GetAngle() * DEGREE);
}

void RotateTowardsCompassAngle(int direction)
{
    const float currentAngle = GetCompassAngle();

    int remainder = static_cast<int>(currentAngle) % Settings::CompassAngleOffset;
    if (direction == 1)
        remainder = Settings::CompassAngleOffset - remainder;

    const float offsetAngle =
        remainder != 0 ? static_cast<float>(remainder * direction) : Settings::CompassAngleOffset * direction;
    player->RotateLocalY(offsetAngle);
}
} // namespace GOTHIC_NAMESPACE
