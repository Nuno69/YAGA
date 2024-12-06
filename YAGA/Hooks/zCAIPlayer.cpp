// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    HOOK Hook_zCAIPlayer_CheckEnoughSpaceMoveForward AS(&zCAIPlayer::CheckEnoughSpaceMoveForward, &zCAIPlayer::Hook_CheckEnoughSpaceMoveForward);
    int zCAIPlayer::Hook_CheckEnoughSpaceMoveForward(const int a)
    {
        if (vob != player)
            return THISCALL(Hook_zCAIPlayer_CheckEnoughSpaceMoveForward)(a);

        float angle = 45.0f;
        zVEC3 vectorAt = vob->GetAtVectorWorld().RotateXZ(angle);
        return CheckEnoughSpaceMoveDir(vectorAt, a);
    }
}