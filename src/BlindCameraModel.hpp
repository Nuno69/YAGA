namespace GOTHIC_NAMESPACE
{
void BlindCameraLoop()
{
    zCVob *cameraVob = ogame->GetCameraVob();
    cameraVob->SetSleeping(TRUE);

    zVEC3 playerHeadPosition = player->GetPositionWorld() + zVEC3(0.0f, 70.0f, 0.0f);
    cameraVob->SetPositionWorld(playerHeadPosition);

    zVEC3 playerAtVector = player->GetAtVectorWorld();
    cameraVob->SetHeadingYWorld(playerHeadPosition + playerAtVector);
}
} // namespace GOTHIC_NAMESPACE