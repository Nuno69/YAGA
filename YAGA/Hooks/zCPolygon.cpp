// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	HOOK Hook_zCPolygon_RenderPoly PATCH(&zCPolygon::RenderPoly, &zCPolygon::RenderPoly_Union);

	int zCPolygon::RenderPoly_Union(int flags) {
		static zVEC3 up(0, 1, 0);
		if (Alg_AngleUnitRad(up, polyPlane.normal) < RAD45)
			return THISCALL(Hook_zCPolygon_RenderPoly)(flags);

		return False;
	}
}