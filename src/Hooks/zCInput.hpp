namespace GOTHIC_NAMESPACE
{
// Lets the navigator hold logical keys (forward, jump) the way a player would, for swimming and for climbing over
// obstacles the engine's NPC movement gives up on. Real key presses still reach the game through the original.
auto Hook_zCInput_Win32_GetState = Union::CreateHook(SIGNATURE_OF(&zCInput_Win32::GetState),
                                                     &zCInput_Win32::GetState_Union, Union::HookType::Hook_Detours);
float zCInput_Win32::GetState_Union(unsigned short logicalKey)
{
    if (Navigator::IsKeyInjected(logicalKey))
        return 1.0f;

    return (this->*Hook_zCInput_Win32_GetState)(logicalKey);
}

auto Hook_zCInput_Win32_GetToggled = Union::CreateHook(
    SIGNATURE_OF(&zCInput_Win32::GetToggled), &zCInput_Win32::GetToggled_Union, Union::HookType::Hook_Detours);
int zCInput_Win32::GetToggled_Union(unsigned short logicalKey)
{
    if (Navigator::TakeInjectedToggle(logicalKey))
        return TRUE;

    return (this->*Hook_zCInput_Win32_GetToggled)(logicalKey);
}

inline const bool RealKeyStateInstalled = []() {
    Navigator::RealKeyState = [](const unsigned short logicalKey) {
        return (static_cast<zCInput_Win32 *>(zinput)->*Hook_zCInput_Win32_GetState)(logicalKey);
    };
    return true;
}();
} // namespace GOTHIC_NAMESPACE
