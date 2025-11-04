namespace GOTHIC_NAMESPACE
{
#if ENGINE <= Engine_G1A
// One time hook that enables Hook_oCWorld_CreateVob.
auto Hook_oCGame_SetupPlayers =
    Union::CreateHook(SIGNATURE_OF(&oCGame::SetupPlayers), &oCGame::Hook_SetupPlayers, Union::HookType::Hook_Detours);
void oCGame::Hook_SetupPlayers(oCNpc *&player, const zSTRING &startPoint)
{
    Hook_oCGame_SetupPlayers.Detach();
    Hook_oCWorld_CreateVob.Commit();

    (this->*Hook_oCGame_SetupPlayers)(player, startPoint);
}
#endif
} // namespace GOTHIC_NAMESPACE