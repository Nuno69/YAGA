// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    HOOK Hook_zCModel_CalcTransBlending AS(&zCModel::CalcTransBlending, &zCModel::Hook_CalcTransBlending);
    void zCModel::Hook_CalcTransBlending()
    {
        THISCALL(Hook_zCModel_CalcTransBlending)();
        if (this == player->GetModel())
        {
            if (zBindPressed(GAME_UP))
            {
                if (zBindPressed(GAME_STRAFELEFT)) this->vobTrans.RotateXZ(+45.0f);
                if (zBindPressed(GAME_STRAFERIGHT)) this->vobTrans.RotateXZ(-45.0f);
            }
        }
    }
}