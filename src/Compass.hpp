namespace GOTHIC_NAMESPACE
{
float NormalizeCompassAngle(float angle)
{
    angle = std::fmod(angle, 360.0f);
    if (angle < 0.0f)
        angle += 360.0f;

    return angle;
}

std::wstring GetCompassName(float _angle)
{
    const int angle = static_cast<int>(NormalizeCompassAngle(_angle));
    const int sector = static_cast<int>((angle + 22) / 45) % 8;

    static const wchar_t *english[] = {L"North",     L"North-east", L"East", L"South-east",
                                       L"South",     L"South-west", L"West", L"North-west"};
    static const wchar_t *polish[] = {L"Północ",     L"Północny wschód", L"Wschód", L"Południowy wschód",
                                      L"Południe",   L"Południowy zachód", L"Zachód", L"Północny zachód"};

    if (Loc::Polish)
        return std::format(L"{}, {}", polish[sector], Loc::Degrees(angle));
    return std::format(L"{} at {}", english[sector], angle);
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
