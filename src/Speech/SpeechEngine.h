#pragma once

class SpeechEngine
{
  protected:
    SpeechEngine(const wchar_t *engineName) : name(engineName)
    {
    }
    SpeechEngine &operator=(const SpeechEngine &)
    {
    }

  public:
    virtual ~SpeechEngine()
    {
    }

    virtual int Read(const wchar_t *message, int interrupt) = 0;
    virtual void StopReading() = 0;
    virtual int IsReady() = 0;

    const wchar_t *GetName()
    {
        return name;
    }

  private:
    const wchar_t *name;
};