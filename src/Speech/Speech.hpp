namespace GOTHIC_NAMESPACE
{
namespace Speech
{
std::unique_ptr<SpeechEngine> engine;

void Initialize(const int engineId)
{
    switch (engineId)
    {
    case 0:
        engine = std::make_unique<SpeechEngineZhengdu>();
        break;

    case 1:
        engine = std::make_unique<SpeechEngineNVDA>();
        break;

    case 2:
        engine = std::make_unique<SpeechEngineSAPI>();
        break;

    default:
        break;
    }
}

void StopReading()
{
    if (engine)
        engine->StopReading();
}

// Every spoken line goes through here, so it is also written to YAGA.log.
int Emit(const std::wstring &text, const int interrupt)
{
    YagaLog::Write(L"[speech] " + text);
    if (!engine)
        return 0;

    return engine->Read(text.c_str(), interrupt);
}

// Read methods

int Read(const char *str, const int interrupt = 1)
{
    return Emit(ToWString(str), interrupt);
}

int Read(const wchar_t *str, const int interrupt = 1)
{
    return Emit(str, interrupt);
}

int Read(const std::string &str, const int interrupt = 1)
{
    return Emit(ToWString(str), interrupt);
}

int Read(const std::wstring &str, const int interrupt = 1)
{
    return Emit(str, interrupt);
}

int Read(const zSTRING &str, const int interrupt = 1)
{
    return Emit(ToWString(str), interrupt);
}

// ReadFormat methods

template <typename... Args> void ReadFormat(std::format_string<Args...> fmt, Args &&...args)
{
    Read(std::format(fmt, std::forward<Args>(args)...));
}
template <typename... Args> void ReadFormatInterrupt(std::format_string<Args...> fmt, Args &&...args)
{
    Read(std::format(fmt, std::forward<Args>(args)...), 1);
}
} // namespace Speech
} // namespace GOTHIC_NAMESPACE
