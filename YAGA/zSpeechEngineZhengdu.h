// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	// Add your code here . . .
	class zSpeechEngineZhengdu : public zSpeechEngine
{
public:
zSpeechEngineZhengdu();
~zSpeechEngineZhengdu();

public:
int Read(const wchar_t *str, int interrupt);
void StopReading();
int IsReady();

private:
typedef int (__stdcall *InitTTS)(int channelType);
typedef void (__stdcall *Speak)(const wchar_t *text, int bInterrupt);
typedef void(__stdcall *StopSpeak)();

private:
HINSTANCE zhengdu;
InitTTS initTTS;
Speak speak;
StopSpeak stopSpeak;
};
}