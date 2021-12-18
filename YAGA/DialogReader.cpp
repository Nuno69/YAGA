// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	HOOK Ivk_HighlightSelected_Union PATCH(&zCViewDialogChoice::HighlightSelected, &zCViewDialogChoice::HighlightSelected_Union);
void zCViewDialogChoice::HighlightSelected_Union()
{
THISCALL(Ivk_HighlightSelected_Union)();
nvdaController_speakText(ConvertToWchar(this->GetSelectedText()));
}
}