// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Prevents knockback damage dealt by dragons. The damage is still taken but the character doesn't start flying.
	HOOK Hook_oCAIHuman_StartFlyDamage PATCH(&oCAIHuman::StartFlyDamage, &oCAIHuman::StartFlyDamage_Union);
	void oCAIHuman::StartFlyDamage_Union(float damage, zVEC3& flyDir)
	{
		if (this != player->human_ai)
			THISCALL(Hook_oCAIHuman_StartFlyDamage)(damage, flyDir);
	}

	// Disables hero turning left/right while holding SHIFT key
	HOOK Hook_oCAIHuman_PC_Turnings AS(&oCAIHuman::PC_Turnings, &oCAIHuman::PC_Turnings_Union);
	void oCAIHuman::PC_Turnings_Union(zBOOL forceRotation)
	{
		if (!zKeyPressed(KEY_LSHIFT) && !zKeyPressed(KEY_RSHIFT))
			THISCALL(Hook_oCAIHuman_PC_Turnings)(forceRotation);
	}
}