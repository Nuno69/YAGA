// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	Array<zSound3D*> zSound3D::Sounds;


	zSound3D::zSound3D() {
		Handle = Invalid;
		Vob = Null;
		Parameters.Obstruction = False;
		Parameters.IsLooped = False;
		Parameters.Radius = 1000.0f;
		Parameters.Pitch = 1.0f;
		Parameters.Volume = 1.0f;
		Parameters.AutoDetele = true;
		Sounds += this;
	}


	zTSound3DParams zSound3D::MakeParameters() {
		zTSound3DParams params;
		params.SetDefaults();
		params.obstruction = Parameters.Obstruction;
		params.loopType = Parameters.IsLooped ? zCSoundSystem::zSND_LOOPING_ENABLED : zCSoundSystem::zSND_LOOPING_DISABLED;
		params.radius = Parameters.Radius;
		params.pitchOffset = 1.0f - Parameters.Pitch;
		params.volume = Parameters.Volume;
		return params;
	}


	zSound3D::Parameters3D zSound3D::ExtractParameters() {
		zTSound3DParams params;
		zsound->GetSound3DProps(Handle, params);
		Parameters3D params3D;
		params3D.Obstruction = params.obstruction;
		params3D.IsLooped = params.loopType == zCSoundSystem::zSND_LOOPING_ENABLED;
		params3D.Radius = params.radius;
		params3D.Pitch = 1.0f + params.pitchOffset;
		params3D.Volume = params.volume;
		return params3D;
	}


	bool zSound3D::UpdateParameters() {
		return UpdateParameters(Parameters);
	}


	bool zSound3D::UpdateParameters(const Parameters3D& params3D) {
		if (IsAvailable()) {
			if (&Parameters != &params3D)
				Parameters = params3D;

			zTSound3DParams params;
			params.obstruction = params3D.Obstruction;
			params.loopType = params3D.IsLooped ? zCSoundSystem::zSND_LOOPING_ENABLED : zCSoundSystem::zSND_LOOPING_DISABLED;
			params.radius = params3D.Radius;
			params.pitchOffset = 1.0f - params3D.Pitch;
			params.volume = params3D.Volume;
			zsound->UpdateSound3D(Handle, &params);
			return true;
		}

		return false;
	}


	bool zSound3D::UpdatePosition() {
		if (IsAvailable()) {
			zsound->UpdateSound3D(Handle, Null);
			return true;
		}

		return false;
	}


	bool zSound3D::UpdatePosition(const zVEC3& position, const float& animation) {
		if (animation > 0.0f) {
			float animation_a = animation;
			float animation_b = 1.0f - animation_a;
			zVEC3 oldPosition = Vob->GetPositionWorld();
			zVEC3 newPosition = (oldPosition * animation_b) + (position * animation_a);
			Vob->SetPositionWorld(newPosition);
		}
		else
			Vob->SetPositionWorld(position);

		return UpdatePosition();
	}


	void zSound3D::Play() {
		Stop();
		zTSound3DParams params = MakeParameters();
		Handle = zsound->PlaySound3D(Sound, Vob, 0, &params);
	}


	void zSound3D::Play(zCVob* vob) {
		if (Vob != Null)
			Vob->Release();

		Vob = vob;
		Vob->AddRef();
		Play();
	}


	void zSound3D::Stop() {
		if (Handle != Invalid) {
			zsound->StopSound(Handle);
			Handle = Invalid;
		}
	}


	bool zSound3D::IsPlays() {
		return Handle != Invalid && zsound->IsSoundActive(Handle);
	}


	bool zSound3D::IsAvailable() {
		return Handle != Invalid && zsound->IsSoundActive(Handle);
	}


	zVEC3 zSound3D::GetPosition() {
		return Vob ? Vob->GetPositionWorld() : 0.0f;
	}


	zSound3D* zSound3D::MakeCopy() {
		zSound3D* sound3D = new zSound3D();
		sound3D->Sound = Sound;
		Sound->AddRef();
		Parameters = Parameters;
		return sound3D;
	}


	zSound3D* zSound3D::CreateSound(const string& fileName) {
		zCSoundFX* sound = zsound->LoadSoundFX(fileName);
		if (!sound)
			return Null;

		zSound3D* sound3D = new zSound3D();
		sound3D->Sound = sound;
		return sound3D;
	}


	zSound3D::~zSound3D() {
		if (Vob)
			Vob->Release();

		if (Sound)
			Sound->Release();

		Sounds -= this;
		cmd << "Sound released" << endl;
	}


	void zSound3D::UpdateAllSounds() {
		static Timer timer;
		if (!timer[1].Await(30))
			return;

		for (uint i = Sounds.GetNum() - 1; i != Invalid; i--) {
			zSound3D* sound = Sounds[i];
			if (!sound->UpdatePosition())
				if (sound->Parameters.AutoDetele)
					sound->Release();
		}
	}


	// class zFreeSound3D

	void zFreeSound3D::Play(const zVEC3& position) {
		if (Vob != Null)
			Vob->Release();

		Vob = new zCVob();
		Vob->SetPositionWorld(position);
		zSound3D::Play();
	}


	zFreeSound3D* zFreeSound3D::MakeCopy() {
		zFreeSound3D* sound3D = new zFreeSound3D();
		sound3D->Sound = Sound;
		Sound->AddRef();
		Parameters = Parameters;
		return sound3D;
	}

	zFreeSound3D* zFreeSound3D::CreateSound(const string& fileName) {
		zCSoundFX* sound = zsound->LoadSoundFX(fileName);
		if (!sound)
			return Null;

		zFreeSound3D* sound3D = new zFreeSound3D();
		sound3D->Sound = sound;
		return sound3D;
	}


	// class zVobSound3D

	bool zVobSound3D::IsAvailable() {
		if (Handle != Invalid)
			return Vob && Vob->homeWorld;

		return true;
	}


	zVobSound3D* zVobSound3D::MakeCopy() {
		zVobSound3D* sound3D = new zVobSound3D();
		sound3D->Sound = Sound;
		Sound->AddRef();
		Parameters = Parameters;
		return sound3D;
	}


	zVobSound3D* zVobSound3D::CreateSound(const string& fileName) {
		zCSoundFX* sound = zsound->LoadSoundFX(fileName);
		if (!sound)
			return Null;

		zVobSound3D* sound3D = new zVobSound3D();
		sound3D->Sound = sound;
		return sound3D;
	}


	zVobSound3D::~zVobSound3D() {
		Stop();
	}


	// class zVobContainerSound3D

	inline int ContainerIsEmpty(oCMobContainer* container) {
		return container->containList.GetNumInList() == 0;
	}


	bool zVobContainerSound3D::IsAvailable() {
		oCMobContainer* container = Vob->CastTo<oCMobContainer>();
		if (container && ContainerIsEmpty(container))
			return false;

		return (Vob && Vob->homeWorld) && zSound3D::IsAvailable();
	}


	zVobContainerSound3D* zVobContainerSound3D::MakeCopy() {
		zVobContainerSound3D* sound3D = new zVobContainerSound3D();
		sound3D->Sound = Sound;
		Sound->AddRef();
		Parameters = Parameters;
		return sound3D;
	}


	zVobContainerSound3D* zVobContainerSound3D::CreateSound(const string& fileName) {
		zCSoundFX* sound = zsound->LoadSoundFX(fileName);
		if (!sound)
			return Null;

		zVobContainerSound3D* sound3D = new zVobContainerSound3D();
		sound3D->Sound = sound;
		return sound3D;
	}
}