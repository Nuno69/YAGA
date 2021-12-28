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

int IsBehind(int angle)
{
if (angle >=0 || angle <=90 || angle >=270 || angle<=359) return 0;
return 1;
}
}