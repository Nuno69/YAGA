// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	HOOK Ivk_HighlightSelected_Union PATCH(&zCViewDialogChoice::HighlightSelected, &zCViewDialogChoice::HighlightSelected_Union);
void zCViewDialogChoice::HighlightSelected_Union()
{
THISCALL(Ivk_HighlightSelected_Union)();
nvdaController_speakText(ConvertToWchar(this->GetSelectedText()));
}
HOOK Ivk_StartSelection_Union PATCH(&zCViewDialogChoice::StartSelection, &zCViewDialogChoice::StartSelection_Union);
  void zCViewDialogChoice::StartSelection_Union()
{
THISCALL(Ivk_StartSelection_Union)();
nvdaController_speakText(ConvertToWchar(zCViewDialogChoice::GetSelectedText()));
}
}