#include <sapi.h>

#include "SpeechEngineSAPI.h"
#include "Settings/YAGA.hpp"

SpeechEngineSAPI::SpeechEngineSAPI() : SpeechEngine(L"SAPI5"),
                                       voice(NULL)
{
	Init();
}

SpeechEngineSAPI::~SpeechEngineSAPI()
{
	UnInit();
}

int SpeechEngineSAPI::Read(const wchar_t* str, int interrupt)
{
	if (!voice) return false;

	// I do these flags so that the voice is more responsive. ASYNC means that the game's thread will not be blocked when the speech is speaking, while SPF_ISNotXML disables the processing of Speech SYnthesis Markup Language (SSML) which has no use here anyway.
	DWORD flags = SPF_ASYNC | SPF_IS_NOT_XML;

	// If interrupt, I will purge the speech queue.
	if (interrupt == 1)
		flags |= SPF_PURGEBEFORESPEAK;

	return SUCCEEDED(voice->Speak(str, flags, NULL));
}

void SpeechEngineSAPI::StopReading()
{
	if (!voice)
		return;

	// TO DO: implement this
}

// SAPI is always ready as it is installed on all Windows versions, hence I return 1 without actual checks.
int SpeechEngineSAPI::IsReady()
{
	return 1;
}

void SpeechEngineSAPI::Init()
{
	// I hate COM. I always did, now I hate it even more. 
	CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&voice);
	
	voice->SetRate(Settings::SpeechSAPIRate);
	voice->SetVolume(Settings::SpeechSAPIVolume);
}

void SpeechEngineSAPI::UnInit()
{
	voice->Release();
	voice = NULL;
}