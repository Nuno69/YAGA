// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

	// Add your code here . . .
	int vCursorPosition = 0;
	int GetNextNPCIndex(int fromWhere, int direction)
{
int tmpfromWhere = fromWhere;
int max = player->vobList.GetNumInList();
int result = -1;
oCNpc *npc = nullptr;
if (fromWhere <= 0 && fromWhere >=max)
{
tmpfromWhere = 0;
}
while (tmpfromWhere <max && npc == nullptr && tmpfromWhere >=0)
{
npc = zDYNAMIC_CAST<oCNpc>(player->vobList[tmpfromWhere]);
if (npc != nullptr)
{
result = tmpfromWhere;
nvdaController_speakText(ConvertToWchar(npc->GetName(1)));
//nvdaController_speakText((wchar_t*)vCursorPosition);
break;
}
else
{
	if (direction = 0)
	{
		tmpfromWhere--;
	}
	else
	{
			tmpfromWhere ++;
	}
}
}
	return result;
}
void GameReaderLoop()
{
	if (zKeyToggled(KEY_COMMA))
	{
vCursorPosition = GetNextNPCIndex(vCursorPosition, 0);
	}
	if (zKeyToggled(KEY_SLASH))
	{
vCursorPosition = GetNextNPCIndex(vCursorPosition, 1);
	}
}
}