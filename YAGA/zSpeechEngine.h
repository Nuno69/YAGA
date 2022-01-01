// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	class zSpeechEngine
{
protected:
zSpeechEngine(const wchar_t *engineName) :
name (engineName) {}
zSpeechEngine& operator = (const zSpeechEngine&) {}

public:
virtual ~zSpeechEngine() {}

public:
virtual int Read(const wchar_t *message, int interrupt) = 0;
virtual void StopReading() = 0;
virtual int IsReady() = 0;

public:
const wchar_t *GetName() {return name;}

private:
const wchar_t *name;
};
}