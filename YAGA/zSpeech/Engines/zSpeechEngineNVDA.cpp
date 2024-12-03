// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Add your code here . . .
	zSpeechEngineNVDA::zSpeechEngineNVDA() :
		zSpeechEngine(L"NVDA"),
		nvda(LoadLibrary("NVDACONTROLLERCLIENT32.dll")),
		nvdaController_speakText(NULL),
		nvdaController_cancelSpeech(NULL),
		nvdaController_testIfRunning(NULL)
	{
		if (nvda)
		{
			nvdaController_speakText = (NVDAController_speakText)GetProcAddress(nvda, "nvdaController_speakText");
			nvdaController_cancelSpeech = (NVDAController_cancelSpeech)GetProcAddress(nvda, "nvdaController_cancelSpeech");
			nvdaController_testIfRunning = (NVDAController_testIfRunning)GetProcAddress(nvda, "nvda_testIfRunning");
		}
	}

	zSpeechEngineNVDA::~zSpeechEngineNVDA()
	{
		if (nvda) FreeLibrary(nvda);
	}

	int zSpeechEngineNVDA::Read(const wchar_t* str, int interrupt)
	{
		if (interrupt == 1) nvdaController_cancelSpeech();
		nvdaController_speakText(str);
		return 1;
	}

	int zSpeechEngineNVDA::IsReady()
	{
		return nvdaController_testIfRunning();
	}

	void zSpeechEngineNVDA::StopReading()
	{
		nvdaController_cancelSpeech();
	}
}