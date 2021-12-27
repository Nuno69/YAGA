// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Add your code here . . .
 oCNpc *selectedNPC;
zCVob *focusedVob;
zCArray<oCNpc*> nearNPC;
static int vCursorPosition = 0;
static bool changed = false;
static bool requestTracking = false;
static bool informedAboutFocusCollect = false;
static bool requestAutoTracking = true;
int numInArray = 0;
void ProcessRadar()
{
	if (nearNPC.GetNum()>0)
	{
nearNPC.EmptyList();
	}
	for(int i = 0; i < player->vobList.GetNum(); i ++)
	{
zCVob *currentVob = player->vobList[i];
oCNpc* currentNPC = zDYNAMIC_CAST<oCNpc>(currentVob);
if (currentNPC)
{
nearNPC.Insert(currentNPC);
}
}
numInArray = nearNPC.GetNum();
}
void RadarLoop()
{
ProcessRadar();
if (zKeyToggled(KEY_COMMA))
{
vCursorPosition --;
changed = true;
}
if (zKeyToggled(KEY_PERIOD))
{
requestTracking = true;
}
if (zKeyToggled(KEY_SLASH))
{
vCursorPosition ++;
changed = true;
}
if (vCursorPosition < 0) vCursorPosition = 0;
else if (vCursorPosition >=numInArray) vCursorPosition = 0;
if (changed)
{
selectedNPC = nearNPC[vCursorPosition];
if (selectedNPC)
{
nvdaController_speakText(ConvertToWchar(selectedNPC->GetName(1)));
zSTRING angle = player->GetAngle(selectedNPC);
nvdaController_speakText(ConvertToWchar(angle));
requestTracking = true;
changed = false;
}
}
if (requestTracking)
{
	if (zCSoundFX* sound = zsound->LoadSoundFX("NPCTARGET.WAV"))
	{
		zCSoundSystem::zTSound3DParams params;
		zsound->GetSound3DProps(0, params);
		zsound->PlaySound3D(sound, selectedNPC, 0, &params);
		sound->Release();
		requestTracking = false;
}
}
if (!informedAboutFocusCollect && player->focus_vob!=nullptr && focusedVob !=player->focus_vob)
{
informedAboutFocusCollect = true;
focusedVob = player->focus_vob;
nvdaController_speakText(ConvertToWchar(focusedVob->GetObjectName()));
}
if (informedAboutFocusCollect && focusedVob !=nullptr)
{
 informedAboutFocusCollect=false;
}

}
}