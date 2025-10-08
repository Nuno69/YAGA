namespace GOTHIC_NAMESPACE {
#if ENGINE <= Engine_G1A
	// One time hook that assigns player ptr before zCVob::ThisVobAddedToWorld gets called.
	auto Hook_oCWorld_CreateVob = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x00745A20, 0x00745A20, 0x00745A20, 0x00745A20)), &oCWorld::Hook_CreateVob, Union::HookType::Hook_Detours);
	oCVob* oCWorld::Hook_CreateVob(const zTVobType type, const zSTRING& name)
	{
		oCVob* result = (this->*Hook_oCWorld_CreateVob)(type, name);
		Hook_oCWorld_CreateVob.Detach();

		if (type == zVOB_TYPE_NSC)
			player = static_cast<oCNpc*>(result);

		return result;
	}
#endif
}