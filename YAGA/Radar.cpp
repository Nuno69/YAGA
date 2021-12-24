// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Add your code here . . .
 oCNpc *selectedNPC;
zCArray<oCNpc*> nearNPC;
static int vCursorPosition = 0;
static bool changed = false;
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
if (zKeyToggled(KEY_SLASH))
{
vCursorPosition ++;
changed = true;
}
if (vCursorPosition < 0) vCursorPosition = 0;
else if (vCursorPosition >=numInArray) vCursorPosition = numInArray-1;
if (changed)
{
selectedNPC = nearNPC[vCursorPosition];
if (selectedNPC)
{
nvdaController_speakText(ConvertToWchar(selectedNPC->GetName(1)));
zSTRING angle = player->GetAngle(selectedNPC)*DEGREE_LONG;
nvdaController_speakText(ConvertToWchar(angle));
changed = false;

}
}
}
}