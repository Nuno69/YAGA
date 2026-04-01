#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace GOTHIC_NAMESPACE
{
struct VobSoundEmitterState
{
    zSTRING fileName;
    int soundHandle = 0;
    float nextPlaybackAt = 0.0f;
};

constexpr float VobSoundMinIntervalMs = 250.0f;
constexpr float VobSoundMaxIntervalMs = 1000.0f;

std::unordered_map<zCVob *, VobSoundEmitterState> vobSoundEmitters;
std::unordered_set<zCVob *> collectedVobSoundEmitters;

float GetCurrentTimeMs()
{
    return ztimer ? ztimer->totalTimeFloat : 0.0f;
}

zCSoundSystem::zTSound3DParams MakeSound3DProps(const int canSee)
{
    zCSoundSystem::zTSound3DParams props;
    props.SetDefaults();
    props.loopType = zCSoundSystem::zSND_LOOPING_DISABLED;
    props.volume = canSee ? 1.0f : 0.3f;
    props.pitchOffset = canSee ? 0.0f : -0.5f;
    props.radius = canSee ? 3500.0f : 500.0f;
    props.reverbLevel = canSee ? 1.0f : 20.0f;
    return props;
}

float GetPlaybackIntervalMs(zCVob *vob, const zCSoundSystem::zTSound3DParams &props)
{
    const float distance = player->GetPositionWorld().Distance(vob->GetPositionWorld());
    const float clampedDistance = std::clamp(distance, 0.0f, props.radius);
    const float distanceAlpha = props.radius > 0.0f ? (clampedDistance / props.radius) : 1.0f;
    return VobSoundMinIntervalMs + (VobSoundMaxIntervalMs - VobSoundMinIntervalMs) * distanceAlpha;
}

bool IsSoundActive(const VobSoundEmitterState &state)
{
    return state.soundHandle != 0 && zsound->IsSoundActive(state.soundHandle);
}

void StopSound3D(VobSoundEmitterState &state)
{
    if (!IsSoundActive(state))
    {
        state.soundHandle = 0;
        return;
    }

    zsound->StopSound(state.soundHandle);
    state.soundHandle = 0;
}

void CollectSound3D(const zSTRING &fileName, zCVob *vob)
{
    auto it = vobSoundEmitters.try_emplace(vob).first;
    it->second.fileName = fileName;

    collectedVobSoundEmitters.insert(vob);
}

void PlaySound3D(VobSoundEmitterState &state, zCVob *vob, zCSoundSystem::zTSound3DParams props)
{
    state.soundHandle = zsound->PlaySound3D(state.fileName, vob, 0, &props);
    state.nextPlaybackAt = GetCurrentTimeMs() + GetPlaybackIntervalMs(vob, props);
}

std::unordered_map<zCVob *, VobSoundEmitterState>::iterator
UnbindSound3D(std::unordered_map<zCVob *, VobSoundEmitterState>::iterator it)
{
    StopSound3D(it->second);
    return vobSoundEmitters.erase(it);
}

void UpdateSoundHandles()
{
    if (!player || !zsound)
        return;

    // clear the collected vob sound emitters, so that they can be populated again in this frame
    collectedVobSoundEmitters.clear();

    // iterate over every collected vob by the player and populate those that should emit the sound
    for (int i = 0; i < player->vobList.GetNumInList(); ++i)
    {
        zCVob *vob = player->vobList[i];

        if (zDYNAMIC_CAST<oCMobContainer>(vob))
            CollectSound3D("WhisperingChest.wav", vob);
        else if (zDYNAMIC_CAST<oCItem>(vob))
            CollectSound3D("WhisperingItem.wav", vob);
        else if (zDYNAMIC_CAST<oCNpc>(vob))
            CollectSound3D("WhisperingNPC.wav", vob);
    }

    // try to update the sound 3d if vob is still present in collectedVobSoundEmitters, if not, remove the tracked vob
    // sound
    for (auto it = vobSoundEmitters.begin(); it != vobSoundEmitters.end();)
    {
        zCVob *vob = it->first;
        VobSoundEmitterState &state = it->second;

        if (collectedVobSoundEmitters.find(vob) == collectedVobSoundEmitters.end())
        {
            it = UnbindSound3D(it);
            continue;
        }

        int canSee = player->CanSee(vob, 1);
        zCSoundSystem::zTSound3DParams props = MakeSound3DProps(canSee);

        if (IsSoundActive(state))
            zsound->UpdateSound3D(state.soundHandle, &props);
        else
            state.soundHandle = 0;

        if (!IsSoundActive(state) && GetCurrentTimeMs() >= state.nextPlaybackAt)
            PlaySound3D(state, vob, props);

        ++it;
    }
}
} // namespace GOTHIC_NAMESPACE
