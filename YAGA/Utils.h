// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	string GetCompassName(int angle) {
		// We compensate the algorithm's inaccuracy by substracting 22 from the angle. This way we will shift our result to the left on the axis.
		int fixedAngle = angle - 22;
		switch (fixedAngle)
		{
		case -22:
			return string("North");
			break;
		case 23:
			return string("North-east");
			break;
		case 68:
			return string("East");
			break;
		case 113:
			return string("South-east");
			break;
		case 158:
			return string("South");
			break;
		case 203:
			return string("South-west");
			break;
		case 248:
			return string("West");
			break;
		case 293:
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