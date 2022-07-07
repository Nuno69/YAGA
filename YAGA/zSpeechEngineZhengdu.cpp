// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Add your code here . . .
	zSpeechEngineZhengdu::zSpeechEngineZhengdu() :
zSpeechEngine(L"ZDSR"),
zhengdu(LoadLibrary("ZDSRAPI.DLL")),
initTTS(NULL),
speak(NULL),
stopSpeak(NULL)
{
		if (zhengdu)
			{
 wchar_t *channelName;
Settings::zSpeechZDUseMultiChannel = (Settings::zSpeechZDUseMultiChannel) ? 0 : 1;
channelName = (Settings::zSpeechZDUseMultiChannel) ? L"Yaga" : NULL;
initTTS = (InitTTS)GetProcAddress(zhengdu, "InitTTS");
speak = (Speak)GetProcAddress(zhengdu, "Speak");
stopSpeak = (StopSpeak)GetProcAddress(zhengdu, "StopSpeak");
initTTS(Settings::zSpeechZDUseMultiChannel, channelName, true);
			}
}
zSpeechEngineZhengdu::~zSpeechEngineZhengdu()
{
if (zhengdu) FreeLibrary(zhengdu);
}
int zSpeechEngineZhengdu::Read(const wchar_t *str, int interrupt)
{
speak(str, interrupt);
return 1;
}
int zSpeechEngineZhengdu::IsReady()
{
return 1;
}
void zSpeechEngineZhengdu::StopReading()
{
stopSpeak();
}
}