// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	string GetCompassName(int angle) {
		switch (angle)
		{
		case 0:
			return string("North");
			break;
		case 45:
			return string("North-east");
			break;
		case 90:
			return string("East");
			break;
		case 135:
			return string("South-east");
			break;
		case 180:
			return string("South");
			break;
		case 225:
			return string("South-west");
			break;
		case 270:
			return string("West");
			break;
		case 315:
			return string("North-west");
			break;
		default:
			return string::Combine("%i degrees", angle);
			break;
		}
	}
	 wstring ComposeReading(zCMenuItem *item)
{
zSTRING itemName = item->GetText(0);
zSTRING itemLabel = item->GetText(1);
return string::Combine("%z: %z", itemName, itemLabel).AToW();
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