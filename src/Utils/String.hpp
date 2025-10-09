namespace GOTHIC_NAMESPACE
{
    std::wstring ToWString(const char* ansi, int length)
    {
        const int wideLength = MultiByteToWideChar(CP_ACP, 0, ansi, length, nullptr, 0);
        if (wideLength == 0)
            return {};

        std::wstring result(wideLength, L'\0');
        MultiByteToWideChar(CP_ACP, 0, ansi, length, &result[0], wideLength);

        return result;
    }

    std::wstring ToWString(const char* str) { return ToWString(str, strlen(str)); }
    std::wstring ToWString(const zSTRING& str) { return ToWString(str.ToChar(), str.Length()); }
    std::wstring ToWString(const std::string& str) { return ToWString(str.c_str(), str.length()); }

    // TO DO: replace this function with generic Read variant!
    std::wstring ComposeReading(zCMenuItem* item)
    {
        const zSTRING itemName = item->GetText(0);
        const zSTRING itemLabel = item->GetText(1);

        return ToWString(std::format("{}: {}", itemName.ToChar(), itemLabel.ToChar()));
    }
}