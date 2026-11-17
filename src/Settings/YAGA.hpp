#ifndef YAGA_SETTINGS
#define YAGA_SETTINGS

#include <string>

namespace Settings
{
// Prism backend for speech: "auto" (the running screen reader, else a speech engine) or a backend name such as NVDA,
// JAWS, ZDSR, SAPI or OneCore.
inline std::string SpeechBackend;
// Rate and volume of a speech engine (SAPI, OneCore), 0-100 with 50 as the default; screen readers keep their own.
inline int SpeechRate;
inline int SpeechVolume;

inline int CompassAngleOffset;
} // namespace Settings

#endif

#ifdef GOTHIC_NAMESPACE

namespace GOTHIC_NAMESPACE
{
void LoadSpeechEngineSettings()
{
    Settings::SpeechBackend = zoptions->ReadString("YAGA", "SpeechBackend", "auto").ToChar();
    Settings::SpeechRate = zoptions->ReadInt("YAGA", "SpeechRate", 50);
    Settings::SpeechVolume = zoptions->ReadInt("YAGA", "SpeechVolume", 50);

    Settings::CompassAngleOffset = zoptions->ReadInt("YAGA", "CompassAngleOffset", 45);
}
} // namespace GOTHIC_NAMESPACE

#endif