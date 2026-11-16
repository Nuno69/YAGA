#ifndef YAGA_SETTINGS
#define YAGA_SETTINGS

namespace Settings
{
inline int SpeechDefaultEngine;
inline int SpeechZDUseMultiChannel;
inline int SpeechSAPIRate;
inline int SpeechSAPIVolume;

inline int CompassAngleOffset;

// 0 = the navigator hands the hero one waypoint at a time (EV_GOTOPOS) and watches every step,
// 1 = the navigator hands the whole route to the engine (EV_GOROUTE) and only watches the result.
inline int NavigatorMode;
// Speak progress every N waypoints while walking (0 = only start, recalculation and arrival).
inline int NavigatorProgressEvery;
// Enables the yaga_cmd.txt command file used for automated testing.
inline int DevCommands;
} // namespace Settings

#endif

#ifdef GOTHIC_NAMESPACE

namespace GOTHIC_NAMESPACE
{
void LoadSpeechEngineSettings()
{
    Settings::SpeechDefaultEngine = zoptions->ReadInt("YAGA", "DefaultSpeechEngine", 1);
    Settings::SpeechZDUseMultiChannel = zoptions->ReadInt("YAGA", "ZDSRUseMultiChannel", 1);
    Settings::SpeechSAPIRate = zoptions->ReadInt("YAGA", "SAPIRate", 0);
    Settings::SpeechSAPIVolume = zoptions->ReadInt("YAGA", "SAPIVolume", 100);

    Settings::CompassAngleOffset = zoptions->ReadInt("YAGA", "CompassAngleOffset", 45);

    Settings::NavigatorMode = zoptions->ReadInt("YAGA", "NavigatorMode", 0);
    Settings::NavigatorProgressEvery = zoptions->ReadInt("YAGA", "NavigatorProgressEvery", 5);
    Settings::DevCommands = zoptions->ReadInt("YAGA", "DevCommands", 0);
}
} // namespace GOTHIC_NAMESPACE

#endif
