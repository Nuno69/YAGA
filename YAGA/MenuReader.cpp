// Supported with union (c) 2020 Union team
// Union SOURCE file
namespace GOTHIC_ENGINE {
HOOK Ivk_PreviousOption_Union PATCH(&zCMenu::PreviousOption, &zCMenu::PreviousOption_Union);
void zCMenu::PreviousOption_Union()
{
THISCALL(Ivk_PreviousOption_Union)();
}
HOOK Ivk_NextOption_Union PATCH(&zCMenu::NextOption, &zCMenu::NextOption_Union);
void zCMenu::NextOption_Union()
{
	THISCALL(Ivk_NextOption_Union)();
}
}