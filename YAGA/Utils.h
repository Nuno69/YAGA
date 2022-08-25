// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	string GetCompassName(int angle) {
		// We compensate the algorithm's inaccuracy by substracting 22 from the angle. This way we will shift our result to the left on the axis.
				 if( angle < 45  - 22 ) return string::Combine( "North at %i", angle );
		else if( angle < 90  - 22 ) return string::Combine( "North-east at %i", angle );
		else if( angle < 135 - 22 ) return string::Combine( "East at %i", angle );
		else if( angle < 180 - 22 ) return string::Combine( "South-east at %i", angle );
		else if( angle < 225 - 22 ) return string::Combine( "South at %i", angle );
		else if( angle < 270 - 22 ) return string::Combine( "South-west at %i", angle );
		else if( angle < 315 - 22 ) return string::Combine( "West at %i", angle );
		else if( angle < 360 - 22 ) return string::Combine( "North-west at %i", angle );
		else return string::Combine( "North at %i", angle );
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