#ifndef PRISM_STATIC
#define PRISM_STATIC
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "SpeechEnginePrism.h"

#include <Windows.h>
#include <algorithm>
#include <format>
#include <prism.h>

#include "Utils/Log.hpp"

namespace
{
// While a speech engine talks (no screen reader was running), look for a screen reader this often, so one started
// after the game takes over.
constexpr unsigned long long LookForScreenReaderMs = 10000;

std::string ToUtf8(const wchar_t *text)
{
    const int length = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1)
        return {};
    std::string result(static_cast<size_t>(length - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, &result[0], length, nullptr, nullptr);
    return result;
}

float Normalized(const int percent)
{
    return std::clamp(percent, 0, 100) / 100.0f;
}

std::string NameOf(PrismBackend *backend)
{
    const char *name = backend ? prism_backend_name(backend) : nullptr;
    return name ? name : "none";
}

// Speech engines (as opposed to screen readers) are known by their backend id.
// Prism's own messages (which libraries it found, why a backend did not start) go to YAGA.log as well.
void PRISM_CALL LogFromPrism(void *, PrismLogLevel level, const char *source, const char *message)
{
    static const char *levels[] = {"trace", "debug", "info", "warn", "error"};
    const char *name = level >= PRISM_LOG_LEVEL_TRACE && level <= PRISM_LOG_LEVEL_ERROR ? levels[level] : "?";
    YagaLog::Write(std::format("[prism] {} {}: {}", name, source ? source : "-", message ? message : ""));
}

bool IsScreenReader(PrismContext *context, PrismBackend *backend)
{
    const char *name = backend ? prism_backend_name(backend) : nullptr;
    const PrismBackendId id = name ? prism_registry_id(context, name) : PRISM_BACKEND_INVALID;
    return id != PRISM_BACKEND_SAPI && id != PRISM_BACKEND_ONE_CORE && id != PRISM_BACKEND_INVALID;
}
} // namespace

SpeechEnginePrism::SpeechEnginePrism(const std::string &backendName, const int rate, const int volume)
    : SpeechEngine(L"Prism"), requested(backendName), rate(Normalized(rate)), volume(Normalized(volume))
{
    // An old ZDSRAPI.dll left in system\Autorun by an earlier YAGA is loaded by Union before Prism looks for the
    // library, and Prism then refuses it (it lacks the Braille function), so ZDSR is never used.
    if (HMODULE zdsr = GetModuleHandleW(L"ZDSRAPI.dll"); zdsr && !GetProcAddress(zdsr, "Braille"))
    {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(zdsr, path, MAX_PATH);
        staleZdsrLibrary = path;
        YagaLog::Write(L"[speech] an old ZDSRAPI.dll keeps Prism from using ZDSR: " + staleZdsrLibrary);
    }

    PrismLogHandler handler{};
    handler.fn = &LogFromPrism;
    prism_set_log_handler(handler);
    prism_set_log_level(PRISM_LOG_LEVEL_TRACE);
    context = prism_init(nullptr);
    Connect();
}

SpeechEnginePrism::~SpeechEnginePrism()
{
    Disconnect();
    if (context)
        prism_shutdown(context);
}

bool SpeechEnginePrism::Connect()
{
    Disconnect();
    lastConnectAttempt = GetTickCount64();
    if (!context)
        return false;

    std::string lower = requested;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    if (!lower.empty() && lower != "auto")
    {
        const PrismBackendId id = prism_registry_id(context, requested.c_str());
        if (id != PRISM_BACKEND_INVALID)
        {
            backend = prism_registry_create(context, id);
            if (backend)
            {
                const PrismError error = prism_backend_initialize(backend);
                if (error != PRISM_OK && error != PRISM_ERROR_ALREADY_INITIALIZED)
                {
                    YagaLog::Write(std::format("[speech] Prism backend {} does not work here: {}", requested,
                                               prism_error_string(error)));
                    prism_backend_free(backend);
                    backend = nullptr;
                }
            }
        }
        else
            YagaLog::Write("[speech] Prism has no backend named " + requested);
    }

    // "auto", or the requested backend does not work here.
    if (!backend)
        backend = prism_registry_create_best(context);
    if (!backend)
    {
        YagaLog::Write("[speech] Prism found no working backend");
        return false;
    }

    Configure();
    return true;
}

void SpeechEnginePrism::Configure()
{
    screenReader = IsScreenReader(context, backend);
    features = prism_backend_get_features(backend);
    brailleFailed = false;
    if (NameOf(backend) != lastBackend)
    {
        lastBackend = NameOf(backend);
        YagaLog::Write(std::format("[speech] Prism backend {} ({})", lastBackend,
                                   screenReader ? "screen reader" : "speech engine"));
    }
    if (screenReader)
        return;

    // Screen readers keep their own voice settings; a speech engine gets the plugin's.
    if (features & PRISM_BACKEND_SUPPORTS_SET_RATE)
        (void)prism_backend_set_rate(backend, rate);
    if (features & PRISM_BACKEND_SUPPORTS_SET_VOLUME)
        (void)prism_backend_set_volume(backend, volume);
}

// Only switches when a screen reader turned up; the speech engine in use keeps talking otherwise.
void SpeechEnginePrism::LookForScreenReader()
{
    lastConnectAttempt = GetTickCount64();
    PrismBackend *best = prism_registry_create_best(context);
    if (!best)
        return;
    if (!IsScreenReader(context, best))
    {
        prism_backend_free(best);
        return;
    }
    Disconnect();
    backend = best;
    Configure();
}

void SpeechEnginePrism::Disconnect()
{
    if (backend)
        prism_backend_free(backend);
    backend = nullptr;
    screenReader = false;
}

int SpeechEnginePrism::Read(const wchar_t *message, const int interrupt)
{
    if (context && !screenReader && GetTickCount64() - lastConnectAttempt > LookForScreenReaderMs)
        LookForScreenReader();
    if (!backend)
        return 0;

    const std::string text = ToUtf8(message);
    if (text.empty())
        return 0;

    // Speech and braille separately (not prism_backend_output): without a braille display some screen readers report
    // the braille part as failed, and that must not count as failed speech.
    PrismError error = prism_backend_speak(backend, text.c_str(), interrupt != 0);
    if (error != PRISM_OK)
    {
        YagaLog::Write(std::format("[speech] {} failed: {}", NameOf(backend), prism_error_string(error)));
        // The screen reader was closed or restarted: connect again (a speech engine if no screen reader runs).
        if (Connect())
            error = prism_backend_speak(backend, text.c_str(), interrupt != 0);
    }

    if (error == PRISM_OK && (features & PRISM_BACKEND_SUPPORTS_BRAILLE) && !brailleFailed)
    {
        const PrismError braille = prism_backend_braille(backend, text.c_str());
        if (braille != PRISM_OK && braille != PRISM_ERROR_NOT_IMPLEMENTED)
        {
            // No braille display, most likely: stop trying until the backend changes.
            brailleFailed = true;
            YagaLog::Write(std::format("[speech] {} braille: {}, braille output off", NameOf(backend),
                                       prism_error_string(braille)));
        }
    }
    return error == PRISM_OK ? 1 : 0;
}

void SpeechEnginePrism::StopReading()
{
    if (backend)
        (void)prism_backend_stop(backend);
}

int SpeechEnginePrism::IsReady()
{
    return backend != nullptr;
}

std::string SpeechEnginePrism::BackendName() const
{
    return NameOf(backend);
}
