namespace GOTHIC_NAMESPACE
{
constexpr float VobSoundMinVolume = 0.1f;
constexpr float VobSoundMaxVolume = 1.0f;

float GetVobSoundVolume(zCVob *vob, const float maxDistance)
{
    if (!player || !vob || maxDistance <= 0.0f)
        return VobSoundMaxVolume;

    const float distance = player->GetPositionWorld().Distance(vob->GetPositionWorld());
    const float distanceAlpha = std::clamp(distance / maxDistance, 0.0f, 1.0f);
    return VobSoundMaxVolume - (VobSoundMaxVolume - VobSoundMinVolume) * distanceAlpha;
}

zCSoundSystem::zTSound3DParams MakeVobSound3DProps(zCVob *vob, const int canSee)
{
    zCSoundSystem::zTSound3DParams props;
    props.SetDefaults();
    props.loopType = zCSoundSystem::zSND_LOOPING_DISABLED;
    props.pitchOffset = canSee ? 0.0f : -0.5f;
    props.radius = canSee ? 3500.0f : 500.0f;
    props.volume = GetVobSoundVolume(vob, props.radius);
    props.reverbLevel = canSee ? 1.0f : 20.0f;
    return props;
}

void SubmitVobSoundCue(zCVob *vob, const char *channel, const zSTRING &fileName)
{
    const int canSee = player->CanSee(vob, 1);
    const zCSoundSystem::zTSound3DParams props = MakeVobSound3DProps(vob, canSee);
    SpatialCues::SubmitVobCue({vob, channel}, fileName, vob, props);
}

void CollectVobSoundCues()
{
    if (!player)
        return;

    for (int i = 0; i < player->vobList.GetNumInList(); ++i)
    {
        zCVob *vob = player->vobList[i];

        if (zDYNAMIC_CAST<oCMobContainer>(vob))
            SubmitVobSoundCue(vob, "container", "WhisperingChest.wav");
        else if (zDYNAMIC_CAST<oCItem>(vob))
            SubmitVobSoundCue(vob, "item", "WhisperingItem.wav");
        else if (zDYNAMIC_CAST<oCNpc>(vob))
            SubmitVobSoundCue(vob, "npc", "WhisperingNPC.wav");
    }
}
} // namespace GOTHIC_NAMESPACE
