// Supported with union (c)2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
#if ENGINE <= Engine_G1A
	// One time hook that assigns player ptr before zCVob::ThisVobAddedToWorld gets called.
	HOOK Hook_oCWorld_CreateVob AS_IF(&oCWorld::CreateVob, &oCWorld::Hook_CreateVob, false);
	oCVob* oCWorld::Hook_CreateVob(const zTVobType type, const zSTRING& name)
	{
		oCVob* result = THISCALL(Hook_oCWorld_CreateVob)(type, name);
		Hook_oCWorld_CreateVob.Detach();

		if (type == zVOB_TYPE_NSC)
			player = static_cast<oCNpc*>(result);

		return result;
	}
#endif
}