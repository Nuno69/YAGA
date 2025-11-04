#pragma once

#include "SpeechEngine.h"
#include <Windows.h>

class SpeechEngineNVDA : public SpeechEngine
{
  public:
    SpeechEngineNVDA();
    ~SpeechEngineNVDA();

  public:
    int Read(const wchar_t *str, int interrupt);
    void StopReading();
    int IsReady();

  private:
    using NVDAController_speakText = int(__stdcall *)(const wchar_t *);
    using NVDAController_cancelSpeech = int(__stdcall *)();
    using NVDAController_testIfRunning = int(__stdcall *)();

  private:
    HINSTANCE nvda;
    NVDAController_speakText nvdaController_speakText;
    NVDAController_cancelSpeech nvdaController_cancelSpeech;
    NVDAController_testIfRunning nvdaController_testIfRunning;
};