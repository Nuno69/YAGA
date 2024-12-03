// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	// Add your code here . . .
	void InitializeSpeech(int engine);
	void UnInitializeSpeech();
	int Read(const wchar_t* str);
	void SilenceSpeech();
}