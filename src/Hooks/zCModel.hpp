namespace GOTHIC_NAMESPACE {
    auto Hook_zCModel_CalcTransBlending = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0057F8E0, 0x0057F8E0, 0x0057F8E0, 0x0057F8E0)), &zCModel::Hook_CalcTransBlending, Union::HookType::Hook_Detours);
    void zCModel::Hook_CalcTransBlending()
    {
        (this->*Hook_zCModel_CalcTransBlending)();
        if (this == player->GetModel())
        {
            if (zinput->KeyPressed(GAME_UP))
            {
                if (zinput->KeyPressed(GAME_STRAFELEFT)) this->vobTrans.RotateXZ(+45.0f);
                if (zinput->KeyPressed(GAME_STRAFERIGHT)) this->vobTrans.RotateXZ(-45.0f);
            }
        }
    }
}