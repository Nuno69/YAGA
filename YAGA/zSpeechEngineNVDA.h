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

};
}