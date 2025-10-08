namespace GOTHIC_NAMESPACE {
	void SimpleSoundPlayback(zSTRING sound, zCVob* vob)
	{
		if (zCSoundFX* snd = zsound->LoadSoundFX(sound))
		{
			zCSoundSystem::zTSound3DParams params;
			zsound->GetSound3DProps(0, params);

			zsound->PlaySound3D(snd, vob, 0, &params);
			snd->Release();
		}
	}
}