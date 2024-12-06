// Supported with union (c)2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	HOOK Hook_zCVob_TraceRay PATCH(&zCVob::TraceRay, &zCVob::TraceRay_Union);

	int zCVob::TraceRay_Union(zVEC3 const& origin, zVEC3 const& ray, int flags, zTTraceRayReport& report) {
		zCVob* vob = this;
		if (!vob->CastTo<oCMobDoor>())
			return False;

		return THISCALL(Hook_zCVob_TraceRay)(origin, ray, flags, report);
	}
}