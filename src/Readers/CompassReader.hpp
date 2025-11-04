namespace GOTHIC_NAMESPACE
{
void CompassReaderLoop()
{
    if (zinput->KeyToggled(KEY_F1))
    {
        float compassAngle = GetCompassAngle();
        Speech::Read(GetCompassName(compassAngle));
    }

    if (zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT))
    {
        if (zinput->KeyToggled(KEY_LEFTARROW))
        {
            RotateTowardsCompassAngle(-1);

            float compassAngle = GetCompassAngle();
            Speech::Read(GetCompassName(compassAngle));
        }

        if (zinput->KeyToggled(KEY_RIGHTARROW))
        {
            RotateTowardsCompassAngle(1);

            float compassAngle = GetCompassAngle();
            Speech::Read(GetCompassName(compassAngle));
        }
    }
}
} // namespace GOTHIC_NAMESPACE