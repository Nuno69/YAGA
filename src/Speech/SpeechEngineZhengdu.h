#pragma once

#include "SpeechEngine.h"
#include <Windows.h>

class SpeechEngineZhengdu : public SpeechEngine
{
public:
	SpeechEngineZhengdu();
	~SpeechEngineZhengdu();

public:
	int Read(const wchar_t* str, int interrupt);
	void StopReading();
	int IsReady();

private:
	using InitTTS = int(__stdcall*)(int type, const wchar_t* channelName, int bKeyDownInterrupt);
	using Speak = void(__stdcall*)(const wchar_t* text, int bInterrupt);
	using StopSpeak = void(__stdcall*)();

private:
	HINSTANCE zhengdu;
	InitTTS initTTS;
	Speak speak;
	StopSpeak stopSpeak;
};