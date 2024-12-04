// Supported with union (c)2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Binds sounds to objects that are spawned in the game world.
	HOOK Hook_zCVob_ThisVobAddedToWorld PATCH(&zCVob::ThisVobAddedToWorld, &zCVob::ThisVobAddedToWorld_Union);
	void zCVob::ThisVobAddedToWorld_Union(zCWorld* world)
	{
		THISCALL(Hook_zCVob_ThisVobAddedToWorld)(world);
		if (this != player) {
			if (CastTo<oCMobContainer>()) {
				BindSound3D("WhisperingChest.wav", this);
			}
			if (CastTo<oCItem>()) {
				int handle = BindSound3D("WhisperingItem.wav", this);
			}
			if (CastTo<oCNpc>()) {
				int handle = BindSound3D("WhisperingNPC.wav", this);
				// 	if( handle != Invalid )
				// 		SoundHandles.Insert( handle );
			}
		}
	}

	// Removes the sound associated with the given vob that is deleted from world.
	HOOK Hook_zCVob_ThisVobRemovedFromWorld PATCH(&zCVob::ThisVobRemovedFromWorld, &zCVob::ThisVobRemovedFromWorld_Union);
	void zCVob::ThisVobRemovedFromWorld_Union(zCWorld* world)
	{
		// TODO
		THISCALL(Hook_zCVob_ThisVobRemovedFromWorld)(world);
	}
}