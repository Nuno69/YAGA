namespace GOTHIC_NAMESPACE
{
auto Hook_zCAIPlayer_CheckEnoughSpaceMoveForward =
    Union::CreateHook(SIGNATURE_OF(&zCAIPlayer::CheckEnoughSpaceMoveForward),
                      &zCAIPlayer::Hook_CheckEnoughSpaceMoveForward, Union::HookType::Hook_Detours);
int zCAIPlayer::Hook_CheckEnoughSpaceMoveForward(const int a)
{
    if (vob != player)
        return (this->*Hook_zCAIPlayer_CheckEnoughSpaceMoveForward)(a);

    float angle = 45.0f;
    zVEC3 vectorAt = vob->GetAtVectorWorld().RotateXZ(angle);
    return CheckEnoughSpaceMoveDir(vectorAt, a);
}
} // namespace GOTHIC_NAMESPACE