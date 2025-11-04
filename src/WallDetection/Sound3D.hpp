// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_NAMESPACE
{
using zTSound3DParams = zCSoundSystem::zTSound3DParams;

class zSound3D : public zCObject
{
  protected:
    static inline zCArray<zSound3D *> Sounds;
    zCSoundFX *Sound = nullptr;
    int Handle = -1;
    zCVob *Vob = nullptr;

    zSound3D() : zCObject()
    {
        Parameters.Obstruction = FALSE;
        Parameters.IsLooped = FALSE;
        Parameters.Radius = 1000.0f;
        Parameters.Pitch = 1.0f;
        Parameters.Volume = 1.0f;
        Parameters.AutoDetele = true;

        Sounds.InsertEnd(this);
    }

    void Play()
    {
        Stop();

        zTSound3DParams params = MakeParameters();
        Handle = zsound->PlaySound3D(Sound, Vob, 0, &params);
    }

    [[nodiscard]] zTSound3DParams MakeParameters() const
    {
        zTSound3DParams params;
        params.SetDefaults();
        params.obstruction = static_cast<float>(Parameters.Obstruction);
        params.loopType =
            Parameters.IsLooped ? zCSoundSystem::zSND_LOOPING_ENABLED : zCSoundSystem::zSND_LOOPING_DISABLED;
        params.radius = Parameters.Radius;
        params.pitchOffset = 1.0f - Parameters.Pitch;
        params.volume = Parameters.Volume;

        return params;
    }

  public:
    struct Parameters3D
    {
        int Obstruction;
        bool IsLooped;
        float Radius;
        float Pitch;
        float Volume;
        bool AutoDetele;
    } Parameters;

    virtual void Play(zCVob *vob)
    {
        if (Vob != nullptr)
            Vob->Release();

        Vob = vob;
        Vob->AddRef();

        Play();
    }

    virtual Parameters3D ExtractParameters()
    {
        zTSound3DParams params;
        zsound->GetSound3DProps(Handle, params);

        Parameters3D params3D{};
        params3D.Obstruction = static_cast<int>(params.obstruction);
        params3D.IsLooped = params.loopType == zCSoundSystem::zSND_LOOPING_ENABLED;
        params3D.Radius = params.radius;
        params3D.Pitch = 1.0f + params.pitchOffset;
        params3D.Volume = params.volume;

        return params3D;
    }

    virtual bool UpdateParameters()
    {
        return UpdateParameters(Parameters);
    }

    virtual bool UpdateParameters(const Parameters3D &params3D)
    {
        if (!IsAvailable())
            return false;

        if (&Parameters != &params3D)
            Parameters = params3D;

        zTSound3DParams params;
        params.obstruction = static_cast<float>(params3D.Obstruction);
        params.loopType =
            params3D.IsLooped ? zCSoundSystem::zSND_LOOPING_ENABLED : zCSoundSystem::zSND_LOOPING_DISABLED;
        params.radius = params3D.Radius;
        params.pitchOffset = 1.0f - params3D.Pitch;
        params.volume = params3D.Volume;

        zsound->UpdateSound3D(Handle, &params);
        return true;
    }

    virtual bool UpdatePosition()
    {
        if (IsAvailable())
        {
            zsound->UpdateSound3D(Handle, nullptr);
            return true;
        }

        return false;
    }

    virtual bool UpdatePosition(const zVEC3 &position, const float &animation = 0.0f)
    {
        if (animation > 0.0f)
        {
            const float animation_a = animation;
            const float animation_b = 1.0f - animation_a;
            const zVEC3 oldPosition = Vob->GetPositionWorld();
            const zVEC3 newPosition = (oldPosition * animation_b) + (position * animation_a);

            Vob->SetPositionWorld(newPosition);
        }
        else
            Vob->SetPositionWorld(position);

        return UpdatePosition();
    }

    void Stop()
    {
        if (Handle == -1)
            return;

        zsound->StopSound(Handle);
        Handle = -1;
    }

    virtual bool IsPlays()
    {
        return Handle != -1 && zsound->IsSoundActive(Handle);
    }

    virtual bool IsAvailable()
    {
        return Handle != -1 && zsound->IsSoundActive(Handle);
    }

    virtual zVEC3 GetPosition()
    {
        return Vob ? Vob->GetPositionWorld() : 0.0f;
    }

    zSound3D *MakeCopy()
    {
        zSound3D *sound3D = new zSound3D();
        sound3D->Sound = Sound;
        Sound->AddRef();

        Parameters = Parameters;

        return sound3D;
    }

    virtual ~zSound3D()
    {
        if (Vob)
            Vob->Release();

        if (Sound)
            Sound->Release();

        Sounds.Remove(this);
        std::cout << "Sound released" << std::endl;
    }

    static zSound3D *CreateSound(const zSTRING &fileName)
    {
        zCSoundFX *sound = zsound->LoadSoundFX(fileName);
        if (!sound)
            return nullptr;

        zSound3D *sound3D = new zSound3D();
        sound3D->Sound = sound;
        return sound3D;
    }

    static void PlayAllSounds()
    {
        for (int i = Sounds.GetNum() - 1; i != -1; --i)
        {
            zSound3D *sound = Sounds[i];
            sound->Play();
        }
    }

    static void StopAllSounds()
    {
        for (int i = Sounds.GetNum() - 1; i != -1; --i)
        {
            zSound3D *sound = Sounds[i];
            sound->Stop();
        }
    }

    static void UpdateAllSounds()
    {
        // static Timer timer;
        // if (!timer[1].Await(30))
        //	return;

        for (int i = Sounds.GetNum() - 1; i != -1; --i)
        {
            zSound3D *sound = Sounds[i];
            if (!sound->UpdatePosition() && sound->Parameters.AutoDetele)
                sound->Release();
        }
    }
};

class zFreeSound3D : public zSound3D
{
  protected:
    zFreeSound3D() : zSound3D()
    {
    }

  public:
    virtual void Play(const zVEC3 &position)
    {
        if (Vob != nullptr)
            Vob->Release();

        Vob = new zCVob();
        Vob->SetPositionWorld(position);
        zSound3D::Play();
    }

    virtual zFreeSound3D *MakeCopy()
    {
        zFreeSound3D *sound3D = new zFreeSound3D();
        sound3D->Sound = Sound;
        Sound->AddRef();

        Parameters = Parameters;

        return sound3D;
    }

    static zFreeSound3D *CreateSound(const zSTRING &fileName)
    {
        zCSoundFX *sound = zsound->LoadSoundFX(fileName);
        if (!sound)
            return nullptr;

        zFreeSound3D *sound3D = new zFreeSound3D();
        sound3D->Sound = sound;
        return sound3D;
    }
};

class zVobSound3D : public zSound3D
{
  protected:
    zVobSound3D() : zSound3D()
    {
        Parameters.IsLooped = TRUE;
    }

  public:
    virtual bool IsAvailable()
    {
        if (Handle != -1)
            return Vob && Vob->homeWorld;

        return true;
    }

    zVobSound3D *MakeCopy()
    {
        zVobSound3D *sound3D = new zVobSound3D();
        sound3D->Sound = Sound;
        Sound->AddRef();

        Parameters = Parameters;

        return sound3D;
    }

    virtual ~zVobSound3D()
    {
        Stop();
    }

    static zVobSound3D *CreateSound(const zSTRING &fileName)
    {
        zCSoundFX *sound = zsound->LoadSoundFX(fileName);
        if (!sound)
            return nullptr;

        zVobSound3D *sound3D = new zVobSound3D();
        sound3D->Sound = sound;

        return sound3D;
    }
};

class zVobContainerSound3D : public zVobSound3D
{
  protected:
    zVobContainerSound3D() : zVobSound3D()
    {
    }

  public:
    virtual bool IsAvailable()
    {
        oCMobContainer *container = Vob->CastTo<oCMobContainer>();
        if (container && container->containList.GetNumInList() == 0)
            return false;

        return (Vob && Vob->homeWorld) && zSound3D::IsAvailable();
    }

    zVobContainerSound3D *MakeCopy()
    {
        zVobContainerSound3D *sound3D = new zVobContainerSound3D();
        sound3D->Sound = Sound;
        Sound->AddRef();

        Parameters = Parameters;

        return sound3D;
    };

    zVobContainerSound3D *CreateSound(const zSTRING &fileName)
    {
        zCSoundFX *sound = zsound->LoadSoundFX(fileName);
        if (!sound)
            return nullptr;

        zVobContainerSound3D *sound3D = new zVobContainerSound3D();
        sound3D->Sound = sound;
        return sound3D;
    }
};
} // namespace GOTHIC_NAMESPACE