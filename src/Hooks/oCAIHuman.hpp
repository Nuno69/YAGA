namespace GOTHIC_NAMESPACE {
	// Prevents knockback damage dealt by dragons. The damage is still taken but the character doesn't start flying.
	auto Hook_oCAIHuman_StartFlyDamage = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0069D940, 0x0069D940, 0x0069D940, 0x0069D940)), &oCAIHuman::StartFlyDamage_Union, Union::HookType::Hook_CallPatch);
	void oCAIHuman::StartFlyDamage_Union(float damage, zVEC3& flyDir)
	{
		if (this != player->human_ai)
			(this->*Hook_oCAIHuman_StartFlyDamage)(damage, flyDir);
	}

	// Disables hero turning left/right while holding SHIFT key
	auto Hook_oCAIHuman_PC_Turnings = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0069A940, 0x0069A940, 0x0069A940, 0x0069A940)), &oCAIHuman::PC_Turnings_Union, Union::HookType::Hook_Detours);
	void oCAIHuman::PC_Turnings_Union(zBOOL forceRotation)
	{
		if (!zinput->KeyPressed(KEY_LSHIFT) && !zinput->KeyPressed(KEY_RSHIFT))
			(this->*Hook_oCAIHuman_PC_Turnings)(forceRotation);
	}
}