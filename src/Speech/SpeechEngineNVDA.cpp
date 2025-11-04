#include "SpeechEngineNVDA.h"

SpeechEngineNVDA::SpeechEngineNVDA()
    : SpeechEngine(L"NVDA"), nvda(LoadLibrary("NVDACONTROLLERCLIENT32.dll")), nvdaController_speakText(NULL),
      nvdaController_cancelSpeech(NULL), nvdaController_testIfRunning(NULL)
{
    if (nvda)
    {
        nvdaController_speakText = (NVDAController_speakText)GetProcAddress(nvda, "nvdaController_speakText");
        nvdaController_cancelSpeech = (NVDAController_cancelSpeech)GetProcAddress(nvda, "nvdaController_cancelSpeech");
        nvdaController_testIfRunning = (NVDAController_testIfRunning)GetProcAddress(nvda, "nvda_testIfRunning");
    }
}

SpeechEngineNVDA::~SpeechEngineNVDA()
{
    if (nvda)
        FreeLibrary(nvda);
}

int SpeechEngineNVDA::Read(const wchar_t *str, int interrupt)
{
    if (interrupt == 1)
        nvdaController_cancelSpeech();
    nvdaController_speakText(str);
    return 1;
}

int SpeechEngineNVDA::IsReady()
{
    return nvdaController_testIfRunning();
}

void SpeechEngineNVDA::StopReading()
{
    nvdaController_cancelSpeech();
}