#include "SpeechEngineZhengdu.h"
#include "Settings/YAGA.hpp"

SpeechEngineZhengdu::SpeechEngineZhengdu()
    : SpeechEngine(L"ZDSR"), zhengdu(LoadLibrary("ZDSRAPI.DLL")), initTTS(NULL), speak(NULL), stopSpeak(NULL)
{
    if (zhengdu)
    {
        Settings::SpeechZDUseMultiChannel = (Settings::SpeechZDUseMultiChannel) ? 0 : 1;
        initTTS = (InitTTS)GetProcAddress(zhengdu, "InitTTS");
        speak = (Speak)GetProcAddress(zhengdu, "Speak");
        stopSpeak = (StopSpeak)GetProcAddress(zhengdu, "StopSpeak");

        const wchar_t *channelName = (Settings::SpeechZDUseMultiChannel) ? L"Yaga" : NULL;
        initTTS(Settings::SpeechZDUseMultiChannel, channelName, true);
    }
}

SpeechEngineZhengdu::~SpeechEngineZhengdu()
{
    if (zhengdu)
        FreeLibrary(zhengdu);
}

int SpeechEngineZhengdu::Read(const wchar_t *str, int interrupt)
{
    speak(str, interrupt);
    return 1;
}

int SpeechEngineZhengdu::IsReady()
{
    return 1;
}

void SpeechEngineZhengdu::StopReading()
{
    stopSpeak();
}