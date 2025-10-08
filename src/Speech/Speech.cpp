#include <iostream>
#include <memory>

#include "SpeechEngineNVDA.h"
#include "SpeechEngineSAPI.h"

std::unique_ptr<SpeechEngine> activeEngine;

void InitializeSpeech(int engine)
{
	switch (engine)
	{
		case 0:
			//activeEngine = std::make_unique<zSpeechEngineZhengdu>();
			break;

		case 1:
			activeEngine = std::make_unique<SpeechEngineNVDA>();
			break;

		case 2:
			activeEngine = std::make_unique<SpeechEngineSAPI>();
			break;

		default:
			break;
	}
}

int Read(const wchar_t *str)
{
	std::wcout << str << std::endl;
	return activeEngine->Read(str, 1);
}

int Read(const wchar_t* str, int interrupt)
{
	std::wcout << str << std::endl;
	return activeEngine->Read( str, interrupt );
}

void SilenceSpeech()
{
	activeEngine->StopReading();
}