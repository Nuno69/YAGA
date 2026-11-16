#ifndef YAGA_LOCALE
#define YAGA_LOCALE

#include <format>
#include <string>

// Spoken messages in the language of the game: Polish for the Polish release, English otherwise. Texts that come
// from the game itself (names, menus, dialogues) are already in the game language.
namespace Loc
{
inline bool Polish = false;
// Code page of the game's own texts. The Polish release uses Windows-1250 whatever the system code page is.
inline unsigned int GameCodePage = 0; // CP_ACP

inline const wchar_t *T(const wchar_t *english, const wchar_t *polish)
{
    return Polish ? polish : english;
}

template <typename... Args>
std::wstring Format(const wchar_t *english, const wchar_t *polish, const Args &...args)
{
    return std::vformat(Polish ? polish : english, std::make_wformat_args(args...));
}

// Polish noun forms: 1 metr, 2-4 metry, 5-21 metrow, 22-24 metry...
inline const wchar_t *PolishPlural(long long count, const wchar_t *one, const wchar_t *few, const wchar_t *many)
{
    if (count < 0)
        count = -count;
    if (count == 1)
        return one;
    const long long units = count % 10;
    const long long tens = count % 100;
    if (units >= 2 && units <= 4 && !(tens >= 12 && tens <= 14))
        return few;
    return many;
}

inline std::wstring Meters(const int count)
{
    if (Polish)
        return std::format(L"{} {}", count, PolishPlural(count, L"metr", L"metry", L"metrów"));
    return std::format(L"{} {}", count, count == 1 ? L"meter" : L"meters");
}

inline std::wstring Waypoints(const int count)
{
    if (Polish)
        return std::format(L"{} {}", count, PolishPlural(count, L"punkt", L"punkty", L"punktów"));
    return std::format(L"{} {}", count, count == 1 ? L"waypoint" : L"waypoints");
}

inline std::wstring Degrees(const int count)
{
    if (Polish)
        return std::format(L"{} {}", count, PolishPlural(count, L"stopień", L"stopnie", L"stopni"));
    return std::format(L"{}", count);
}
} // namespace Loc

#endif

#ifdef GOTHIC_NAMESPACE

namespace GOTHIC_NAMESPACE
{
// [YAGA] Language = auto | pl | en. "auto" looks at a script constant that every release translates: NAME_VALUE
// is "Wartość:" in the Polish release and "Value:" in the English one. Without scripts it falls back to the Windows
// language.
inline void DetectLanguage(std::string setting)
{
    std::transform(setting.begin(), setting.end(), setting.begin(), [](unsigned char c) { return std::tolower(c); });

    std::string source;
    if (setting == "pl" || setting == "polish")
    {
        Loc::Polish = true;
        source = "setting";
    }
    else if (setting == "en" || setting == "english")
    {
        Loc::Polish = false;
        source = "setting";
    }
    else
    {
        zCPar_Symbol *symbol = parser ? parser->GetSymbol(zSTRING("NAME_VALUE")) : nullptr;
        if (symbol)
        {
            zSTRING value;
            symbol->GetValue(value, 0);
            const std::string text = value.ToChar();
            Loc::Polish = text.rfind("Warto", 0) == 0;
            source = "game text \"" + text + "\"";
        }
        else
        {
            Loc::Polish = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_POLISH;
            source = "Windows language";
        }
    }

    Loc::GameCodePage = Loc::Polish ? 1250 : CP_ACP;
    YagaLog::Write(std::format("[locale] {} (from {})", Loc::Polish ? "Polish" : "English", source));
}
} // namespace GOTHIC_NAMESPACE

#endif
