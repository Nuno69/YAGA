// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	// Add your code here . . .
namespace Settings
{
int zSpeechDefaultEngine, zSpeechZDUseMultiChannel;
void SpeechEngine()
{
zSpeechDefaultEngine = zoptions->ReadInt("YAGA", "DefaultSpeechEngine", 1);
zSpeechZDUseMultiChannel = zoptions->ReadInt("YAGA", "ZDSRUseMultiChannel", 1);
}
}
}