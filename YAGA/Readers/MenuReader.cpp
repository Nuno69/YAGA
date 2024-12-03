// Supported with union (c) 2020 Union team
// Union SOURCE file
namespace GOTHIC_ENGINE {
	// Reads a menu item focused by pressing the up arrow key.
	HOOK Ivk_PreviousOption_Union PATCH(&zCMenu::PreviousOption, &zCMenu::PreviousOption_Union);
	void zCMenu::PreviousOption_Union()
	{
		THISCALL(Ivk_PreviousOption_Union)();
		Read(ComposeReading(zCMenu::GetActiveItem()));
	}
	// Reads a menu item focused by pressing the down arrow key.
	HOOK Ivk_NextOption_Union PATCH(&zCMenu::NextOption, &zCMenu::NextOption_Union);
	void zCMenu::NextOption_Union()
	{
		THISCALL(Ivk_NextOption_Union)();
		Read(ComposeReading(zCMenu::GetActiveItem()));
	}
}