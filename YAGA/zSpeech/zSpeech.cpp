// Supported with union (c) 2020 Union team
// Union SOURCE file
#include <memory>

namespace GOTHIC_ENGINE {
	wstring lastRead = Null;
	std::unique_ptr<zSpeechEngine> activeEngine;

	void InitializeSpeech(int engine)
	{
		switch (engine)
		{
			case 0:
				activeEngine = std::make_unique<zSpeechEngineZhengdu>();
				break;

			case 1:
				activeEngine = std::make_unique<zSpeechEngineNVDA>();
				break;

			case 2:
				activeEngine = std::make_unique<zSpeechEngineSAPI>();
				break;

			default:
				break;
		}
	}

	int Read(const wchar_t *str)
	{
		cmd << str << endl;
		lastRead = str;
		return activeEngine->Read(str, 1);
	}

	int Read(const wchar_t* str, int interrupt)
	{
		cmd << str << endl;
		lastRead = str;
		return activeEngine->Read( str, interrupt );
	}

	void SilenceSpeech()
	{
		activeEngine->StopReading();
	}
}