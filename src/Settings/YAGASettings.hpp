namespace Settings
{
	inline int SpeechDefaultEngine;
	inline int SpeechZDUseMultiChannel;
	inline int SpeechSAPIRate;
	inline int SpeechSAPIVolume;

	inline int CompassAngleOffset;
}

#ifdef GOTHIC_NAMESPACE

namespace GOTHIC_NAMESPACE
{
	void SpeechEngine()
	{
		Settings::SpeechDefaultEngine = zoptions->ReadInt("YAGA", "DefaultSpeechEngine", 1);
		Settings::SpeechZDUseMultiChannel = zoptions->ReadInt("YAGA", "ZDSRUseMultiChannel", 1);
		Settings::SpeechSAPIRate = zoptions->ReadInt("YAGA", "SAPIRate", 0);
		Settings::SpeechSAPIVolume = zoptions->ReadInt("YAGA", "SAPIVolume", 100);

		Settings::CompassAngleOffset = zoptions->ReadInt("YAGA", "CompassAngleOffset", 45);
	}
}

#endif