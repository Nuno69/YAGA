namespace GOTHIC_NAMESPACE
{
// Reads the selected dialog choice.
auto Hook_zCViewDialogChoice_HighlightSelected =
    Union::CreateHook(SIGNATURE_OF(&zCViewDialogChoice::HighlightSelected),
                      &zCViewDialogChoice::HighlightSelected_Union, Union::HookType::Hook_CallPatch);
void zCViewDialogChoice::HighlightSelected_Union()
{
    (this->*Hook_zCViewDialogChoice_HighlightSelected)();
    Speech::Read(this);
}

// Reads the default dialog choice when a dialog box is presented.
auto Hook_zCViewDialogChoice_StartSelection =
    Union::CreateHook(SIGNATURE_OF(&zCViewDialogChoice::StartSelection), &zCViewDialogChoice::StartSelection_Union,
                      Union::HookType::Hook_CallPatch);
void zCViewDialogChoice::StartSelection_Union()
{
    (this->*Hook_zCViewDialogChoice_StartSelection)();
    Speech::Read(this);
}
} // namespace GOTHIC_NAMESPACE