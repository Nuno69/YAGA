// Supported with union (c) 2020 Union team
// Union SOURCE file

#include <unordered_set>
#include <unordered_map>

namespace GOTHIC_ENGINE {
	std::unordered_map<zCVob*, int> vob_soundHandle;
	std::unordered_set<zCVob*> collectedVobSoundEmitters;

	bool BindSound3D(const string& fileName, zCVob* vob)
	{
		auto it = vob_soundHandle.find(vob);
		if (it != vob_soundHandle.end())
		{
			collectedVobSoundEmitters.insert(vob);
			return true;
		}

		zCSoundFX* sfx = zsound->LoadSoundFX(fileName);
		if (!sfx)
			return false;

		int handle = zsound->PlaySound3D(sfx, vob, 0, 0);
		if (handle != 0)
		{
			sfx->SetLooping(True);

			collectedVobSoundEmitters.insert(vob);
			vob_soundHandle.insert({ vob, handle });
		}

		sfx->Release();
		return handle != 0;
	}

	std::unordered_map<zCVob*, int>::iterator UnbindSound3D(std::unordered_map<zCVob*, int>::iterator it, zCVob* vob)
	{
		zsound->StopSound(it->second);
		return vob_soundHandle.erase(it);
	}

	void UpdateSoundHandles()
	{
		collectedVobSoundEmitters.clear();

		for (int i = 0; i < player->vobList.GetNumInList(); ++i)
		{
			zCVob* vob = player->vobList[i];

			if (zDYNAMIC_CAST<oCMobContainer>(vob)) BindSound3D("WhisperingChest.wav", vob);
			else if (zDYNAMIC_CAST<oCItem>(vob)) BindSound3D("WhisperingItem.wav", vob);
			else if (zDYNAMIC_CAST<oCNpc>(vob)) BindSound3D("WhisperingNPC.wav", vob);
		}

		for (auto it = vob_soundHandle.begin(); it != vob_soundHandle.end(); )
		{
			zCVob* vob = it->first;
			int soundHandle = it->second;

			if (collectedVobSoundEmitters.find(vob) == collectedVobSoundEmitters.end())
			{
				it = UnbindSound3D(it, vob);
				continue;
			}

			int canSee = player->CanSee(vob, 1);

			zCSoundSystem::zTSound3DParams props;
			zsound->GetSound3DProps(soundHandle, props);
			props.volume = canSee ? 1.0f : 0.3f;
			props.pitchOffset = canSee ? 0.0f : -0.5f;
			props.radius = canSee ? 3500.0f : 500.0f;
			props.reverbLevel = canSee ? 1.0f : 20.0f;

			zsound->UpdateSound3D(soundHandle, &props);

			++it;
		}
	}
}