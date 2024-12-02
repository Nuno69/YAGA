// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	HOOK Hook_oCAIHuman_StartFlyDamage PATCH(&oCAIHuman::StartFlyDamage, &oCAIHuman::StartFlyDamage_Union);

	void oCAIHuman::StartFlyDamage_Union(float damage, zVEC3& flyDir) {
		if (this != player->human_ai)
			THISCALL(Hook_oCAIHuman_StartFlyDamage)(damage, flyDir);
	}
}