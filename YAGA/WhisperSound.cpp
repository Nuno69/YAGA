// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	Array<int> SoundHandles;

	void UpdateSoundHandles() {
		for each (int handle in SoundHandles) {
			zCActiveSnd* snd = zCActiveSnd::GetHandleSound(handle);
			if (!snd) {
				zsound->UpdateSound3D(handle, Null);
				continue;
			}

			if (!snd->sourceVob)
				continue;

			int canSee = player->CanSee(snd->sourceVob, 1);

			zCSoundSystem::zTSound3DParams props;
			zsound->GetSound3DProps(handle, props);
			props.volume = canSee ? 1.0f : 0.3f;
			props.pitchOffset = canSee ? 0.0f : -0.5f;
			props.radius = canSee ? 3500.0f : 500.0f;
			props.reverbLevel = canSee ? 1.0f : 20.0f;

			zsound->UpdateSound3D(handle, &props);
		}
	}

	int BindSound3D(const string& fileName, zCVob* vob) {
		zCSoundFX* sfx = zsound->LoadSoundFX(fileName);
		if (!sfx)
			return Invalid;

		sfx->SetLooping(True);
		int handle = zsound->PlaySound3D(sfx, vob, 0, 0);
		sfx->Release();
		if (handle != Invalid)
			SoundHandles.Insert(handle);

		return handle;
	}
	// This hook binds sounds to objects that are spawned in the game world.
	HOOK Hook_zCVob_ThisVobAddedToWorld PATCH(&zCVob::ThisVobAddedToWorld, &zCVob::ThisVobAddedToWorld_Union);

	void zCVob::ThisVobAddedToWorld_Union(zCWorld* world) {
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

	HOOK Hook_zCVob_ThisVobRemovedFromWorld PATCH(&zCVob::ThisVobRemovedFromWorld, &zCVob::ThisVobRemovedFromWorld_Union);
	// This hook removes the sound associated with the given vob that is deleted from world.
	void zCVob::ThisVobRemovedFromWorld_Union(zCWorld* world) {
		// TODO
		THISCALL(Hook_zCVob_ThisVobRemovedFromWorld)(world);
	}
}