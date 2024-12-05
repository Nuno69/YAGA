// Supported with union (c)2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Binds sounds to objects that are spawned in the game world.
	HOOK Hook_zCVob_ThisVobAddedToWorld AS(&zCVob::ThisVobAddedToWorld, &zCVob::Hook_ThisVobAddedToWorld);
	void zCVob::Hook_ThisVobAddedToWorld(zCWorld* world)
	{
		THISCALL(Hook_zCVob_ThisVobAddedToWorld)(world);

		if (this != player)
		{
			if (CastTo<oCMobContainer>()) BindSound3D("WhisperingChest.wav", this);
			else if (CastTo<oCItem>()) BindSound3D("WhisperingItem.wav", this);
			else if (CastTo<oCNpc>()) BindSound3D("WhisperingNPC.wav", this);
		}
	}

	// Removes the sound associated with the given vob that is deleted from world.
	HOOK Hook_zCVob_ThisVobRemovedFromWorld AS(&zCVob::ThisVobRemovedFromWorld, &zCVob::Hook_ThisVobRemovedFromWorld);
	void zCVob::Hook_ThisVobRemovedFromWorld(zCWorld* world)
	{
		THISCALL(Hook_zCVob_ThisVobRemovedFromWorld)(world);

		if (this != player)
		{
			if (CastTo<oCMobContainer>()) UnbindSound3D(this);
			else if (CastTo<oCItem>()) UnbindSound3D(this);
			else if (CastTo<oCNpc>()) UnbindSound3D(this);
		}
	}
}