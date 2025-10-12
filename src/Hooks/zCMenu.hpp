namespace GOTHIC_NAMESPACE {
	// Reads a menu item focused by pressing the up arrow key.
	auto Hook_zCMenu_PreviousOption = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x004DB5C0, 0x004DB5C0, 0x004DB5C0, 0x004DB5C0)), &zCMenu::PreviousOption_Union, Union::HookType::Hook_CallPatch);
	void zCMenu::PreviousOption_Union()
	{
		(this->*Hook_zCMenu_PreviousOption)();
		Speech::Read(this->GetActiveItem());
	}

	// Reads a menu item focused by pressing the down arrow key.
	auto Hook_zCMenu_NextOption = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x004DB760, 0x004DB760, 0x004DB760, 0x004DB760)), &zCMenu::NextOption_Union, Union::HookType::Hook_CallPatch);
	void zCMenu::NextOption_Union()
	{
		(this->*Hook_zCMenu_NextOption)();
		Speech::Read(this->GetActiveItem());
	}
}