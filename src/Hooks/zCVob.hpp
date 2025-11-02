namespace GOTHIC_NAMESPACE {
	auto Hook_zCVob_TraceRay = Union::CreateHook(SIGNATURE_OF(&zCVob::TraceRay), &zCVob::Hook_TraceRay);
	int zCVob::Hook_TraceRay(zVEC3 const& origin, zVEC3 const& ray, int flags, zTTraceRayReport& report)
	{
		if (!this->CastTo<oCMobDoor>())
			return FALSE;

		return this->*Hook_zCVob_TraceRay)(origin, ray, flags, report);
	}
}