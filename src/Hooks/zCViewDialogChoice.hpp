namespace GOTHIC_NAMESPACE
{
using zCViewDialogChoiceFastHook = Union::Hook<void (__fastcall zCViewDialogChoice::*)()>;

inline zCViewDialogChoiceFastHook Hook_zCViewDialogChoice_HighlightSelected(nullptr);
inline zCViewDialogChoiceFastHook Hook_zCViewDialogChoice_StartSelection(nullptr);

inline void InitializeDialogChoiceHooks()
{
    static bool initialized = false;
    if (initialized)
        return;

    Hook_zCViewDialogChoice_HighlightSelected =
        Union::CreateHook(reinterpret_cast<void *>(zSwitch(0x007594A0, 0x0079BA50, 0x007A6380, 0x0068F620)),
                          &zCViewDialogChoice::HighlightSelected_Union, Union::HookType::Hook_Detours);
    Hook_zCViewDialogChoice_StartSelection =
        Union::CreateHook(reinterpret_cast<void *>(zSwitch(0x00758E00, 0x0079B3C0, 0x007A5CD0, 0x0068EF70)),
                          &zCViewDialogChoice::StartSelection_Union, Union::HookType::Hook_Detours);

    initialized = true;
}

// Reads the selected dialog choice.
void zCViewDialogChoice::HighlightSelected_Union()
{
    (this->*Hook_zCViewDialogChoice_HighlightSelected)();
    Speech::Read(this);
}

// Reads the default dialog choice when a dialog box is presented.
void zCViewDialogChoice::StartSelection_Union()
{
    (this->*Hook_zCViewDialogChoice_StartSelection)();
    Speech::Read(this);
}
} // namespace GOTHIC_NAMESPACE
