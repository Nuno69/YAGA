#pragma once

#include "SpeechEngine.h"

#include <string>

struct PrismContext;
struct PrismBackend;

// Speech through Prism (https://github.com/ethindp/prism): the running screen reader (NVDA, JAWS, ZDSR and others),
// or a speech engine (SAPI, OneCore) when no screen reader runs. Text goes to braille as well where the screen
// reader supports it.
class SpeechEnginePrism : public SpeechEngine
{
  public:
    // backendName: "auto" picks the best backend that works; otherwise a Prism backend name ("NVDA", "JAWS", "ZDSR",
    // "SAPI", "OneCore"...). rate and volume (0-100, 50 = default) apply to speech engines; screen readers keep their
    // own settings.
    SpeechEnginePrism(const std::string &backendName, int rate, int volume);
    ~SpeechEnginePrism();

    int Read(const wchar_t *message, int interrupt) override;
    void StopReading() override;
    int IsReady() override;

    // Name of the backend in use, for the log.
    std::string BackendName() const;

    // Path of an outdated ZDSRAPI.dll loaded into the game (it blocks the ZDSR backend), or empty.
    const std::wstring &StaleZdsrLibrary() const
    {
        return staleZdsrLibrary;
    }

  private:
    bool Connect();
    void Configure();
    void LookForScreenReader();
    void Disconnect();

    PrismContext *context = nullptr;
    PrismBackend *backend = nullptr;
    std::string requested;
    float rate;
    float volume;
    bool screenReader = false;
    unsigned long long features = 0;
    bool brailleFailed = false;
    std::string lastBackend;
    unsigned long long lastConnectAttempt = 0;
    std::wstring staleZdsrLibrary;
};
