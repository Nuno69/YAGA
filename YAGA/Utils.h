// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	const wchar_t *ConvertToWchar(const char *s)
{
const size_t strLength = strlen(s) +1;
wchar_t *result = new wchar_t [strLength];
mbstowcs(result, s, strLength);
return result;
}
	 zSTRING ComposeReading(zCMenuItem *item)
{
zSTRING itemName = item->GetText(0);
zSTRING itemLabel = item->GetText(1);
return itemName+ ":" + itemLabel;
}

int GetAngleRelation(zCVob* source, zCVob* target)
{
zVEC3 position = (source->GetPositionWorld() - target->GetPositionWorld().Normalize());
zVEC3 at = source->GetAtVectorWorld();
float radian = Alg_AngleUnitRad(position, at);
return (radian *DEGREE_LONG);
}
zSTRING GetPlayerFacing()
{
return player->human_ai->GetFootPos();
}
const string GetWordForAngle(int angle)
{
//	SWITCH (ANGLE)
//	{
//CASE 0:
//RETURN "N";
//BREAK;
//CASE 1:
//RETURN "N AT "+ANGLE+" DEGREE";
//BREAK;
//CASE 2 ... 44:
//RETURN "NE AT "+ANGLE+" DEGREES";
//BREAK;
//CASE 45:
//RETURN "NE";
//BREAK;
//CASE 46 ... 89:
//RETURN "NE AT "+ANGLE+" DEGREES";
//BREAK;
//CASE 90:
//RETURN "E";
//BREAK;
//CASE 91 ... 134:
//RETURN "SE AT "+ANGLE+" DEGREES";
//BREAK;
//CASE 135:
//RETURN "SE";
//BREAK;
//CASE 136 ... 179:
//RETURN "SE AT "+ANGLE+" DEGREES";
//BREAK;
//CASE 180:
//RETURN "S";
//BREAK;
//CASE 181 ... 224:
//RETURN "SW AT "+ANGLE+" DEGREES";
//BREAK;
//CASE 225:
//RETURN "SW";
//BREAK;
//CASE 226 ... 269:
//RETURN "SW AT "+ANGLE+" DEGREES";
//BREAK;
//CASE 270:
//RETURN "W";
//BREAK;
//CASE 271 ... 314:
//RETURN "NW AT "+ANGLE+" DEGREES";
//BREAK;
//CASE 315:
//RETURN "NW";
//BREAK;
//CASE 316 ... 359:
//RETURN "NW AT "+ANGLE+" DEGREES";
//BREAK;
//	DEFAULT:
//		BREAK;
//	}
}
}