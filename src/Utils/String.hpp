namespace GOTHIC_NAMESPACE
{
// Game texts use the game's code page (Windows-1250 for the Polish release), not necessarily the system one.
std::wstring ToWString(const char *ansi, int length)
{
    const int wideLength = MultiByteToWideChar(Loc::GameCodePage, 0, ansi, length, nullptr, 0);
    if (wideLength == 0)
        return {};

    std::wstring result(wideLength, L'\0');
    MultiByteToWideChar(Loc::GameCodePage, 0, ansi, length, &result[0], wideLength);

    return result;
}

std::wstring ToWString(const char *str)
{
    return ToWString(str, strlen(str));
}
std::wstring ToWString(const std::string &str)
{
    return ToWString(str.c_str(), str.length());
}
std::wstring ToWString(const zSTRING &str)
{
    return ToWString(str.ToChar(), str.Length());
}
} // namespace GOTHIC_NAMESPACE