#pragma once

// TO DO:
// Move this to namespace
// 
// Speech::Initialize
// Speech::Read
// Speech::Stop

void InitializeSpeech(int engine);
int Read(const wchar_t* str);
int Read(const wchar_t* str, int interrupt);
void SilenceSpeech();