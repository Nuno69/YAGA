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
break;
case 1:
activeEngine = new zSpeechEngineNVDA();
			default:
				break;
		}
}
void UnInitializeSpeech()
{
delete(activeEngine);
}
int Read(const wchar_t *str, int interrupt=1)
{
return activeEngine->Read(str, interrupt);
}
void SilenceSpeech()
{
activeEngine->StopReading();
}
}