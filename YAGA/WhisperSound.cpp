// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	Array<int> SoundHandles;

	void UpdateSoundHandles()
	{
		for each (int handle in SoundHandles)
		{
			zCActiveSnd* snd = zCActiveSnd::GetHandleSound(handle);
			if (!snd)
			{
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

	int BindSound3D(const string& fileName, zCVob* vob)
	{
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
}