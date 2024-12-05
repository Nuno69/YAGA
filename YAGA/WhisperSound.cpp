// Supported with union (c) 2020 Union team
// Union SOURCE file

#include <unordered_map>
#include "WhisperSound.h"

namespace GOTHIC_ENGINE {
	std::unordered_map<zCVob*, int> SoundHandles;

	void UpdateSoundHandles()
	{
		for each (std::pair<zCVob*, int> pair in SoundHandles)
		{
			zCActiveSnd* snd = zCActiveSnd::GetHandleSound(pair.second);
			if (!snd)
			{
				zsound->UpdateSound3D(pair.second, nullptr);
				continue;
			}

			if (!snd->sourceVob)
				continue;

			int canSee = player->CanSee(snd->sourceVob, 1);

			zCSoundSystem::zTSound3DParams props;
			zsound->GetSound3DProps(pair.second, props);
			props.volume = canSee ? 1.0f : 0.3f;
			props.pitchOffset = canSee ? 0.0f : -0.5f;
			props.radius = canSee ? 3500.0f : 500.0f;
			props.reverbLevel = canSee ? 1.0f : 20.0f;

			zsound->UpdateSound3D(pair.second, &props);
		}
	}

	bool BindSound3D(const string& fileName, zCVob* vob)
	{
		zCSoundFX* sfx = zsound->LoadSoundFX(fileName);
		if (!sfx)
			return false;

		int handle = zsound->PlaySound3D(sfx, vob, 0, 0);
		if (handle != Invalid)
		{
			sfx->SetLooping(True);
			SoundHandles.insert({ vob, handle });
		}

		sfx->Release();
		return handle != Invalid;
	}

	bool UnbindSound3D(zCVob* vob)
	{
		auto it = SoundHandles.find(vob);
		if (it == SoundHandles.end())
			return false;

		zsound->StopSound(it->second);
		SoundHandles.erase(it);

		return true;
	}
}