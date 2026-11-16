namespace GOTHIC_NAMESPACE
{
auto Hook_zCAIPlayer_CheckEnoughSpaceMoveForward =
    Union::CreateHook(SIGNATURE_OF(&zCAIPlayer::CheckEnoughSpaceMoveForward),
                      &zCAIPlayer::Hook_CheckEnoughSpaceMoveForward, Union::HookType::Hook_Detours);
int zCAIPlayer::Hook_CheckEnoughSpaceMoveForward(const int a)
{
    // The navigator walks the hero straight at its target, so it needs the engine's own forward check.
    if (vob != player || Navigator::Active)
        return (this->*Hook_zCAIPlayer_CheckEnoughSpaceMoveForward)(a);

    float angle = 45.0f;
    zVEC3 vectorAt = vob->GetAtVectorWorld().RotateXZ(angle);
    return CheckEnoughSpaceMoveDir(vectorAt, a);
}
} // namespace GOTHIC_NAMESPACE