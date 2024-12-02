// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	// Add your code here . . .
	namespace Settings
	{
		int zSpeechDefaultEngine;
		int zSpeechZDUseMultiChannel;
		int zSpeechSAPIRate;
		int zSpeechSAPIVolume;

		int zCompassAngleOffset;

		void SpeechEngine()
		{
			zSpeechDefaultEngine = zoptions->ReadInt("YAGA", "DefaultSpeechEngine", 1);
			zSpeechZDUseMultiChannel = zoptions->ReadInt("YAGA", "ZDSRUseMultiChannel", 1);
			zSpeechSAPIRate = zoptions->ReadInt("YAGA", "SAPIRate", 0);
			zSpeechSAPIVolume = zoptions->ReadInt("YAGA", "SAPIVolume", 100);

			zCompassAngleOffset = zoptions->ReadInt("YAGA", "CompassAngleOffset", 45);
		}
	}
}