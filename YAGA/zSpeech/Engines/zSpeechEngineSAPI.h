// Supported with union (c) 2020 Union team
// Union HEADER file
#include <sapi.h>
namespace GOTHIC_ENGINE {
	// Add your code here . . .
	class zSpeechEngineSAPI : public zSpeechEngine
	{
	public:
		zSpeechEngineSAPI();
		~zSpeechEngineSAPI();
	public:
		int Read(const wchar_t* str, int interrupt);
		void StopReading();
		int IsReady();
	private:
		void Init();
		void UnInit();
	private:
		ISpVoice* voice;
	};
}