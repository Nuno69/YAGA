// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	const wchar_t *ConvertToWchar(const char *s)
{
const size_t strLength = strlen(s) +1;
wchar_t *result = new wchar_t [strLength];
mbstowcs(result, s, strLength);
return result;
}
}