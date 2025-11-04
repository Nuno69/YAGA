#pragma once

#include "SpeechEngine.h"

struct ISpVoice;

class SpeechEngineSAPI : public SpeechEngine
{
  public:
    SpeechEngineSAPI();
    ~SpeechEngineSAPI();

  public:
    int Read(const wchar_t *str, int interrupt);
    void StopReading();
    int IsReady();

  private:
    void Init();
    void UnInit();

  private:
    ISpVoice *voice;
};