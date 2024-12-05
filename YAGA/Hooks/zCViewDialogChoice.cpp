// Supported with union (c)2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Reads the selected dialog choice.
	HOOK Ivk_HighlightSelected_Union PATCH(&zCViewDialogChoice::HighlightSelected, &zCViewDialogChoice::HighlightSelected_Union);
	void zCViewDialogChoice::HighlightSelected_Union()
	{
		THISCALL(Ivk_HighlightSelected_Union)();
		ReadDialogChoice(this);
	}

	// Reads the default dialog choice when a dialog box is presented.
	HOOK Ivk_StartSelection_Union PATCH(&zCViewDialogChoice::StartSelection, &zCViewDialogChoice::StartSelection_Union);
	void zCViewDialogChoice::StartSelection_Union()
	{
		THISCALL(Ivk_StartSelection_Union)();
		ReadDialogChoice(this);
	}
}