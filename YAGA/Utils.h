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
//const string GetWordForAngle(int angle)
//{
//	switch (angle)
//	{
//case 0:
//return "N";
//break;
//case 1:
//return "N at "+angle+" degree";
//break;
//case 2 ... 44:
//return "NE at "+angle+" degrees";
//break;
//case 45:
//return "NE";
//break;
//case 46 ... 89:
//return "NE at "+angle+" degrees";
//break;
//case 90:
//return "E";
//break;
//case 91 ... 134:
//return "SE at "+angle+" degrees";
//break;
//case 135:
//return "SE";
//break;
//case 136 ... 179:
//return "SE at "+angle+" degrees";
//break;
//case 180:
//return "S";
//break;
//case 181 ... 224:
//return "SW at "+angle+" degrees";
//break;
//case 225:
//return "SW";
//break;
//case 226 ... 269:
//return "SW at "+angle+" degrees";
//break;
//case 270:
//return "W";
//break;
//case 271 ... 314:
//return "NW at "+angle+" degrees";
//break;
//case 315:
//return "NW";
//break;
//case 316 ... 359:
//return "NW at "+angle+" degrees";
//break;
//	default:
//		break;
//	}
//}
}