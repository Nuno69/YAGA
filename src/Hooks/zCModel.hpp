namespace GOTHIC_NAMESPACE
{
auto Hook_zCModel_CalcTransBlending = Union::CreateHook(
    SIGNATURE_OF(&zCModel::CalcTransBlending), &zCModel::Hook_CalcTransBlending, Union::HookType::Hook_Detours);
void zCModel::Hook_CalcTransBlending()
{
    (this->*Hook_zCModel_CalcTransBlending)();
    if (this == player->GetModel())
    {
        if (zinput->KeyPressed(GAME_UP))
        {
            if (zinput->KeyPressed(GAME_STRAFELEFT))
                this->vobTrans.RotateXZ(+45.0f);
            if (zinput->KeyPressed(GAME_STRAFERIGHT))
                this->vobTrans.RotateXZ(-45.0f);
        }
    }
}
} // namespace GOTHIC_NAMESPACE