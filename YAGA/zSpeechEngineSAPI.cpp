// Supported with union (c) 2020 Union team
// Union SOURCE file
namespace GOTHIC_ENGINE {
	// Add your code here . . .
	zSpeechEngineSAPI::zSpeechEngineSAPI() :
		zSpeechEngine(L"SAPI5"),
		voice(NULL)
	{
		Init();
	}
	zSpeechEngineSAPI::~zSpeechEngineSAPI()
	{
		UnInit();
	}
	int zSpeechEngineSAPI::Read(const wchar_t* str, int interrupt)
	{
		if (!voice) return false;
		// I do these flags so that the voice is mroe responsive. ASYNC means that the game's thread will not be blocked when the speech is speaking, while SPF_ISNotXML disables the processing of Speech SYnthesis Markup Language (SSML) which has no use here anyway.
		DWORD flags = SPF_ASYNC | SPF_IS_NOT_XML;
		// If interrupt, I will purge the speech queue.
		if (interrupt == 1) flags |= SPF_PURGEBEFORESPEAK;
		return SUCCEEDED(voice->Speak(str, flags, NULL));
	}
	void zSpeechEngineSAPI::StopReading()
	{
		if (!voice) return;
		const DWORD flags = SPF_ASYNC | SPF_IS_NOT_XML | SPF_PURGEBEFORESPEAK;
		return;
	}
	// SAPI is always ready as it is installed on all WIndows versions, hence I return 1 without actual checks.
	int zSpeechEngineSAPI::IsReady()
	{
		return 1;
	}
	void zSpeechEngineSAPI::Init()
	{
		// I hate COM. I always did, now I hate it even more. 
		CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&voice);
		return;
	}
	void zSpeechEngineSAPI::UnInit()
	{
		voice->Release();
		voice = NULL;
	}
}