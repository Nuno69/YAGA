#include <algorithm>
#include <unordered_map>

namespace GOTHIC_NAMESPACE
{
namespace SpatialCues
{
using zTSound3DParams = zCSoundSystem::zTSound3DParams;

enum class Cadence
{
    Loop,
    DistancePulse
};

struct DistancePulseSettings
{
    float NearIntervalMs = 250.0f;
    float FarIntervalMs = 1000.0f;
    float MaxDistance = 0.0f;
};

struct Key
{
    const void *Owner = nullptr;
    const char *Channel = nullptr;

    bool operator==(const Key &other) const
    {
        return Owner == other.Owner && Channel == other.Channel;
    }
};

struct KeyHash
{
    size_t operator()(const Key &key) const
    {
        return std::hash<const void *>()(key.Owner) ^ (std::hash<const char *>()(key.Channel) << 1);
    }
};

struct Spec
{
    zSTRING FileName;
    zTSound3DParams Params;
    Cadence CadenceType = Cadence::DistancePulse;
    DistancePulseSettings Pulse;
    zCVob *AnchorVob = nullptr;
    zVEC3 AnchorPosition = 0.0f;
    bool UseWorldPosition = false;
};

struct State
{
    Spec Cue;
    int SoundHandle = 0;
    float NextPlaybackAt = 0.0f;
    bool TouchedThisFrame = false;
    zCVob *WorldAnchorVob = nullptr;
};

std::unordered_map<Key, State, KeyHash> ActiveCues;

float GetCurrentTimeMs()
{
    return ztimer ? ztimer->totalTimeFloat : 0.0f;
}

void Stop(State &state)
{
    if (state.SoundHandle != 0 && zsound->IsSoundActive(state.SoundHandle))
        zsound->StopSound(state.SoundHandle);

    state.SoundHandle = 0;
}

void Release(State &state)
{
    Stop(state);

    if (!state.WorldAnchorVob)
        return;

    state.WorldAnchorVob->Release();
    state.WorldAnchorVob = nullptr;
}

zCVob *ResolveAnchor(State &state)
{
    if (!state.Cue.UseWorldPosition)
        return state.Cue.AnchorVob;

    if (!state.WorldAnchorVob)
        state.WorldAnchorVob = new zCVob();

    state.WorldAnchorVob->SetPositionWorld(state.Cue.AnchorPosition);
    return state.WorldAnchorVob;
}

bool IsHandleActive(const State &state)
{
    return state.SoundHandle != 0 && zsound->IsSoundActive(state.SoundHandle);
}

float GetPlaybackIntervalMs(State &state)
{
    zCVob *anchor = ResolveAnchor(state);
    if (!anchor || !player)
        return state.Cue.Pulse.FarIntervalMs;

    const float maxDistance = state.Cue.Pulse.MaxDistance > 0.0f ? state.Cue.Pulse.MaxDistance : state.Cue.Params.radius;
    if (maxDistance <= 0.0f)
        return state.Cue.Pulse.NearIntervalMs;

    const float distance = player->GetPositionWorld().Distance(anchor->GetPositionWorld());
    const float distanceAlpha = std::clamp(distance / maxDistance, 0.0f, 1.0f);
    return state.Cue.Pulse.NearIntervalMs +
           (state.Cue.Pulse.FarIntervalMs - state.Cue.Pulse.NearIntervalMs) * distanceAlpha;
}

void Play(State &state)
{
    zCVob *anchor = ResolveAnchor(state);
    if (!anchor)
        return;

    zTSound3DParams params = state.Cue.Params;
    state.SoundHandle = zsound->PlaySound3D(state.Cue.FileName, anchor, 0, &params);

    if (state.Cue.CadenceType == Cadence::DistancePulse)
        state.NextPlaybackAt = GetCurrentTimeMs() + GetPlaybackIntervalMs(state);
}

State &Touch(const Key &key)
{
    State &state = ActiveCues[key];
    state.TouchedThisFrame = true;
    return state;
}

void SubmitVobCue(const Key &key, const zSTRING &fileName, zCVob *anchorVob, const zTSound3DParams &params,
                  const DistancePulseSettings &pulse = {})
{
    State &state = Touch(key);
    state.Cue.FileName = fileName;
    state.Cue.Params = params;
    state.Cue.CadenceType = Cadence::DistancePulse;
    state.Cue.Pulse = pulse;
    state.Cue.AnchorVob = anchorVob;
    state.Cue.UseWorldPosition = false;
}

void SubmitWorldCue(const Key &key, const zSTRING &fileName, const zVEC3 &position, const zTSound3DParams &params,
                    const Cadence cadence, const DistancePulseSettings &pulse = {})
{
    State &state = Touch(key);
    state.Cue.FileName = fileName;
    state.Cue.Params = params;
    state.Cue.CadenceType = cadence;
    state.Cue.Pulse = pulse;
    state.Cue.AnchorPosition = position;
    state.Cue.UseWorldPosition = true;
}

void BeginFrame()
{
    for (auto &[key, state] : ActiveCues)
        state.TouchedThisFrame = false;
}

void StopAll()
{
    for (auto &[key, state] : ActiveCues)
        Stop(state);
}

void UpdateAll()
{
    if (!zsound)
        return;

    for (auto it = ActiveCues.begin(); it != ActiveCues.end();)
    {
        State &state = it->second;
        if (!state.TouchedThisFrame)
        {
            Release(state);
            it = ActiveCues.erase(it);
            continue;
        }

        zCVob *anchor = ResolveAnchor(state);
        if (!anchor || (!state.Cue.UseWorldPosition && anchor->homeWorld == nullptr))
        {
            Release(state);
            it = ActiveCues.erase(it);
            continue;
        }

        if (IsHandleActive(state))
            zsound->UpdateSound3D(state.SoundHandle, &state.Cue.Params);
        else
            state.SoundHandle = 0;

        switch (state.Cue.CadenceType)
        {
        case Cadence::Loop:
            if (!IsHandleActive(state))
                Play(state);
            break;
        case Cadence::DistancePulse:
            if (!IsHandleActive(state) && GetCurrentTimeMs() >= state.NextPlaybackAt)
                Play(state);
            break;
        }

        ++it;
    }
}
} // namespace SpatialCues
} // namespace GOTHIC_NAMESPACE
