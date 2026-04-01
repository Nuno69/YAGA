namespace GOTHIC_NAMESPACE
{
using zCMenuVoidHook = Union::Hook<void (zCMenu::*)()>;

inline zCMenuVoidHook Hook_zCMenu_Enter(nullptr);
inline zCMenuVoidHook Hook_zCMenu_PreviousOption(nullptr);
inline zCMenuVoidHook Hook_zCMenu_NextOption(nullptr);

inline void InitializeMenuHooks()
{
    static bool initialized = false;
    if (initialized)
        return;

    Hook_zCMenu_Enter = Union::CreateHook(reinterpret_cast<void *>(zSwitch(0x004CEB90, 0x004DF420, 0x004D90E0, 0x004DB780)),
                                          &zCMenu::Enter_Union, Union::HookType::Hook_Detours);
    Hook_zCMenu_PreviousOption =
        Union::CreateHook(reinterpret_cast<void *>(zSwitch(0x004CE9D0, 0x004DF260, 0x004D8F20, 0x004DB5C0)),
                          &zCMenu::PreviousOption_Union, Union::HookType::Hook_Detours);
    Hook_zCMenu_NextOption =
        Union::CreateHook(reinterpret_cast<void *>(zSwitch(0x004CEAA0, 0x004DF330, 0x004D8FF0, 0x004DB690)),
                          &zCMenu::NextOption_Union, Union::HookType::Hook_Detours);

    initialized = true;
}

// Read the initially focused item when a menu opens.
void zCMenu::Enter_Union()
{
    (this->*Hook_zCMenu_Enter)();
    Speech::Read(this->GetActiveItem());
}

// Read the item focused by pressing the up arrow key.
void zCMenu::PreviousOption_Union()
{
    (this->*Hook_zCMenu_PreviousOption)();
    Speech::Read(this->GetActiveItem());
}

// Read the item focused by pressing the down arrow key.
void zCMenu::NextOption_Union()
{
    (this->*Hook_zCMenu_NextOption)();
    Speech::Read(this->GetActiveItem());
}
} // namespace GOTHIC_NAMESPACE
