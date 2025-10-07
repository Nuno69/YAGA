// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	typedef zCSoundSystem::zTSound3DParams zTSound3DParams;


	class zSound3D : public zCObject {
	protected:
		static Array<zSound3D*> Sounds;
		zCSoundFX* Sound;
		int Handle;
		zCVob* Vob;

		zSound3D();
		void Play();
		zTSound3DParams MakeParameters();
	public:
		struct Parameters3D {
			int Obstruction;
			bool IsLooped;
			float Radius;
			float Pitch;
			float Volume;
			bool AutoDetele;
		}
		Parameters;

		virtual void Play(zCVob* vob);
		virtual Parameters3D ExtractParameters();
		virtual bool UpdateParameters();
		virtual bool UpdateParameters(const Parameters3D& params);
		virtual bool UpdatePosition();
		virtual bool UpdatePosition(const zVEC3& position, const float& animation = 0.0f);
		virtual void Stop();
		virtual bool IsPlays();
		virtual bool IsAvailable();
		virtual zVEC3 GetPosition();
		zSound3D* MakeCopy();
		virtual ~zSound3D();

		static zSound3D* CreateSound(const string& fileName);
		static void UpdateAllSounds();
	};


	class zFreeSound3D : public zSound3D {
	protected:
		zFreeSound3D() : zSound3D() {}
	public:
		virtual void Play(const zVEC3& position);
		virtual zFreeSound3D* MakeCopy();
		static zFreeSound3D* CreateSound(const string& fileName);
	};


	class zVobSound3D : public zSound3D {
	protected:
		zVobSound3D() : zSound3D() { Parameters.IsLooped = True; }
	public:
		virtual bool IsAvailable();
		zVobSound3D* MakeCopy();
		virtual ~zVobSound3D();
		static zVobSound3D* CreateSound(const string& fileName);
	};


	class zVobContainerSound3D : public zVobSound3D {
	protected:
		zVobContainerSound3D() : zVobSound3D() {}
	public:
		virtual bool IsAvailable();
		zVobContainerSound3D* MakeCopy();
		static zVobContainerSound3D* CreateSound(const string& fileName);
	};
}