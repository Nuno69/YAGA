namespace GOTHIC_NAMESPACE {
	// Reads the selected dialog choice.
	auto Hook_zCViewDialogChoice_HighlightSelected = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0068F620, 0x0068F620, 0x0068F620, 0x0068F620)), &zCViewDialogChoice::HighlightSelected_Union, Union::HookType::Hook_CallPatch);
	void zCViewDialogChoice::HighlightSelected_Union()
	{
	    (this->*Hook_zCViewDialogChoice_HighlightSelected)();
		//ReadDialogChoice(this);
	}

	// Reads the default dialog choice when a dialog box is presented.
	auto Hook_zCViewDialogChoice_StartSelection = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0068EF70, 0x0068EF70, 0x0068EF70, 0x0068EF70)), &zCViewDialogChoice::StartSelection_Union, Union::HookType::Hook_CallPatch);
	void zCViewDialogChoice::StartSelection_Union()
	{
		(this->*Hook_zCViewDialogChoice_StartSelection)();
		//ReadDialogChoice(this);
	}
}