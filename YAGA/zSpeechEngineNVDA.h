// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	// Add your code here . . .
	class zSpeechEngineNVDA : public zSpeechEngine
{
public:
zSpeechEngineNVDA();
~zSpeechEngineNVDA();

public:
	int Read(const wchar_t* str, int interrupt);
	void StopReading();
	int IsReady();

private:
typedef int (__stdcall *NVDAController_speakText)(const wchar_t *);
typedef int (__stdcall *NVDAController_cancelSpeech)();
typedef int (__stdcall *NVDAController_testIfRunning)();

private:
HINSTANCE nvda;
NVDAController_speakText nvdaController_speakText;
NVDAController_cancelSpeech nvdaController_cancelSpeech;
NVDAController_testIfRunning nvdaController_testIfRunning;
};
}