// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	HOOK Hook_zCPolygon_RenderPoly AS(&zCPolygon::RenderPoly, &zCPolygon::Hook_RenderPoly);
	int zCPolygon::Hook_RenderPoly(int flags)
	{
		static zVEC3 up(0, 1, 0);
		if (Alg_AngleUnitRad(up, polyPlane.normal) < RAD45)
			return THISCALL(Hook_zCPolygon_RenderPoly)(flags);

		return False;
	}
}