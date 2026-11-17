#ifndef YAGA_LOG
#define YAGA_LOG

#include <cstdio>
#include <mutex>
#include <share.h>
#include <string>

// Session log written to YAGA.log next to the game executable. It is truncated on every start and records spoken
// lines plus navigator events, so a player's report can be diagnosed from the file alone.
namespace YagaLog
{
inline std::mutex Mutex;
inline FILE *File = nullptr;
inline bool OpenFailed = false;

inline void Write(const std::wstring &line)
{
    std::lock_guard<std::mutex> lock(Mutex);

    if (!File && !OpenFailed)
    {
        // Shared, so the log can be read while the game runs (_wfopen_s would lock it).
        File = _wfsopen(L"YAGA.log", L"w, ccs=UTF-8", _SH_DENYNO);
        if (!File)
            OpenFailed = true;
    }

    if (!File)
        return;

    SYSTEMTIME time;
    GetLocalTime(&time);
    fwprintf(File, L"%02d:%02d:%02d.%03d %s\n", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds,
             line.c_str());
    fflush(File);
}

inline void Write(const std::string &line)
{
    const int wideLength = MultiByteToWideChar(CP_ACP, 0, line.c_str(), static_cast<int>(line.length()), nullptr, 0);
    std::wstring wide(wideLength, L'\0');
    if (wideLength > 0)
        MultiByteToWideChar(CP_ACP, 0, line.c_str(), static_cast<int>(line.length()), &wide[0], wideLength);
    Write(wide);
}
} // namespace YagaLog

#endif
