// Supported with union (c) 2020 Union team
// Union SOURCE file
namespace GOTHIC_ENGINE {
	// Add your code here . . .
zSpeechEngine *activeEngine;
void InitializeSpeech(int engine)
{
	switch (engine)
		{
case 0:
activeEngine = new zSpeechEngineZhengdu();
			default:
				break;
		}
}
void UnInitializeSpeech()
{
delete(activeEngine);
}
int Read(const wchar_t *str)
{
return activeEngine->Read(str, 1);
}
void SilenceSpeech()
{
activeEngine->StopReading();
}
}