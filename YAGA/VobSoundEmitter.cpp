// Supported with union (c) 2020 Union team
// Union SOURCE file

#include <unordered_set>
#include <unordered_map>

namespace GOTHIC_ENGINE {
	std::unordered_map<zCVob*, int> vobSoundEmitters;
	std::unordered_set<zCVob*> collectedVobSoundEmitters;

	bool BindSound3D(const string& fileName, zCVob* vob)
	{
		auto it = vobSoundEmitters.find(vob);
		if (it != vobSoundEmitters.end())
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
			vobSoundEmitters.insert({ vob, handle });
		}

		sfx->Release();
		return handle != 0;
	}

	std::unordered_map<zCVob*, int>::iterator UnbindSound3D(std::unordered_map<zCVob*, int>::iterator it, zCVob* vob)
	{
		zsound->StopSound(it->second);
		return vobSoundEmitters.erase(it);
	}

	void UpdateSoundHandles()
	{
		// clear the collected vob sound emitters, so that they can be populated again in this frame
		collectedVobSoundEmitters.clear();

		// iterate over every collected vob by the player and populate those that should emit the sound
		for (int i = 0; i < player->vobList.GetNumInList(); ++i)
		{
			zCVob* vob = player->vobList[i];

			if (zDYNAMIC_CAST<oCMobContainer>(vob)) BindSound3D("WhisperingChest.wav", vob);
			else if (zDYNAMIC_CAST<oCItem>(vob)) BindSound3D("WhisperingItem.wav", vob);
			else if (zDYNAMIC_CAST<oCNpc>(vob)) BindSound3D("WhisperingNPC.wav", vob);
		}

		// try to update the sound 3d if vob is still present in collectedVobSoundEmitters, if not, remove the tracked vob sound
		for (auto it = vobSoundEmitters.begin(); it != vobSoundEmitters.end(); )
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