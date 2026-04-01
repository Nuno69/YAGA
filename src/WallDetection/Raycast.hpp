namespace GOTHIC_NAMESPACE
{
const float StepLength = 50.0f;
const float StepHeight = 50.0f;
const zVEC3 zRIGHT(1.0f, 0.0f, 0.0f);
const zVEC3 zUP(0.0f, 1.0f, 0.0f);
const zVEC3 zAT(0.0f, 0.0f, 1.0f);

bool StaticTraceRay(const zVEC3 &origin, const zVEC3 &ray, zVEC3 *intersection = nullptr, zVEC3 *normal = nullptr)
{
    oCWorld *world = ogame->GetGameWorld();
    int hitFound = world->TraceRayNearestHit(origin, ray, player, zTRACERAY_STAT_POLY | zTRACERAY_POLY_NORMAL);
    if (!hitFound)
    {
        if (intersection)
            *intersection = origin + ray;
        return false;
    }

    if (intersection)
        *intersection = world->traceRayReport.foundIntersection;
    if (normal)
        *normal = world->traceRayReport.foundPolyNormal;
    return true;
}

void Line3D(const zVEC3 &pointA, const zVEC3 &pointB, const zCOLOR &color = GFX_WHITE)
{
    zlineCache->Line3D(pointA, pointB, color, 0);
}

void Line3D(const zVEC3 &origin, const zCOLOR &color = GFX_WHITE)
{
    Line3D(origin, origin + zUP * 5.0f, color);
}

inline zVEC3 GetFarSoundPosition()
{
    return player->GetPositionWorld() + player->GetAtVectorWorld() * 850.0f;
}

inline zVEC3 MakeVector(const zVEC3 &from, const zVEC3 &to, const float &length)
{
    return (to - from).Normalize() * length;
}

inline float GetGradient(const float &value, const float &max)
{
    float x = value / max;
    return value * sqrt(x);
}

zCSoundSystem::zTSound3DParams MakeWorldCueProps(const float radius, const float volume = 1.0f)
{
    zCSoundSystem::zTSound3DParams props;
    props.SetDefaults();
    props.loopType = zCSoundSystem::zSND_LOOPING_DISABLED;
    props.radius = radius;
    props.volume = volume;
    return props;
}

void WallLoop(bool detected, const zVEC3 &position = GetFarSoundPosition(), const zVEC3 &normal = zVEC3())
{
    if (!detected)
        return;

    static const char WallFarCueToken = 0;
    static const char WallNearCueToken = 0;

    zVEC3 positionHero = player->GetPositionWorld();
    zVEC3 hitVector = position - positionHero;
    float hitDistance = GetGradient(hitVector.Length(), 1000.0f);
    zVEC3 positionNorm = positionHero + (hitVector * 0.5f) - (normal * hitDistance * 2.0f);
    zVEC3 positionBest = positionHero + MakeVector(positionHero, positionNorm, hitDistance);
    positionBest[VY] = ogame->GetCameraVob()->GetPositionWorld()[VY];

    SpatialCues::SubmitWorldCue({&WallFarCueToken, "wall-far"}, "WallHumFar.wav", positionBest, MakeWorldCueProps(900.0f),
                                SpatialCues::Cadence::DistancePulse);
    SpatialCues::SubmitWorldCue({&WallNearCueToken, "wall-near"}, "WallHumNear.wav", positionBest,
                                MakeWorldCueProps(120.0f), SpatialCues::Cadence::DistancePulse);
}

void ChasmLoop(bool detected, const zVEC3 &position = zVEC3())
{
    if (!detected)
        return;

    static const char ChasmCueToken = 0;
    SpatialCues::SubmitWorldCue({&ChasmCueToken, "chasm"}, "Chasm.wav", position, MakeWorldCueProps(1500.0f, 5.0f),
                                SpatialCues::Cadence::DistancePulse);
}

bool GetFloorAt(const zVEC3 &origin, zVEC3 &result)
{
    return StaticTraceRay(origin + zUP * StepHeight, zUP * (-150.0f - StepHeight * 2.0f), &result);
}

float GetHeight(const zVEC3 &origin)
{
    zVEC3 result;
    if (!StaticTraceRay(origin + zUP * StepHeight, zUP * -10000.0f, &result))
        return 10000.0f;

    return origin.Distance(result);
}

bool CheckNextStep(const zVEC3 &origin, const zVEC3 &vector, zVEC3 &result)
{
    zVEC3 firstStep = origin + zUP * 5.0f;
    if (!GetFloorAt(firstStep, firstStep))
        return false;

    zVEC3 secondStep = (firstStep) + (vector * StepLength);
    if (!GetFloorAt(secondStep, result))
    {
        result = secondStep;
        return false;
    }

    return true;
}

bool CheckWall(const zVEC3 &from, const zVEC3 &to, zVEC3 &wallPosition, zVEC3 &wallNormal)
{
    float height_alpha = max(from[VY], to[VY] + 1.0f);
    zVEC3 from_a = zVEC3(from[VX], height_alpha, from[VZ]);
    zVEC3 to_a = zVEC3(to[VX], height_alpha, to[VZ]);

    zVEC3 ray = to_a - from_a;
    for (float y = 100.0f; y < 185.0f; y += 5.0f)
        if (StaticTraceRay(from_a + zUP * y, ray, &wallPosition, &wallNormal))
            return true;

    for (float y = 95.0f; y > 45.0f; y -= 5.0f)
        if (StaticTraceRay(from_a + zUP * y, ray, &wallPosition, &wallNormal))
            return true;

    return false;
}

bool CastStepRays(int stepsCount, const zVEC3 &origin, const zVEC3 &vector, zVEC3 &wallPosition, zVEC3 &wallNormal,
                  float &chasmHeight)
{
    chasmHeight = 0.0f;
    zVEC3 firstStep = origin;
    do
    {
        zVEC3 secondStep;
        Line3D(firstStep, GFX_YELLOW);
        bool floorFound = CheckNextStep(firstStep, vector, secondStep);
        if (CheckWall(firstStep, secondStep, wallPosition, wallNormal))
            return true;

        if (!floorFound)
        {
            wallPosition = secondStep;
            chasmHeight = GetHeight(secondStep);
            return true;
        }

        Line3D(firstStep, secondStep, GFX_GREEN);
        firstStep = secondStep;
    } while (--stepsCount);

    return false;
}

void CheckAreaAtFront()
{
    zVEC3 origin = player->GetPositionWorld();
    zVEC3 vector = player->GetAtVectorWorld();
    zVEC3 wallPosition;
    zVEC3 wallNormal;
    float chasmHeight;
    bool pathIsBlocked = CastStepRays(20, origin, vector, wallPosition, wallNormal, chasmHeight);
    if (!pathIsBlocked)
    {
        // Free way
        WallLoop(false);
        ChasmLoop(false);
        screen->Print(500, 500, "Free way");
    }
    else if (chasmHeight > 0.0f)
    {
        // Chasm
        WallLoop(false);
        ChasmLoop(true, wallPosition);
        Line3D(wallPosition, wallPosition - zUP * chasmHeight, GFX_RED);
        screen->Print(500, 500, "Chasm found " + zSTRING(chasmHeight));
    }
    else
    {
        // Wall
        WallLoop(true, wallPosition, wallNormal);
        ChasmLoop(false);
        screen->Print(500, 500, "Wall found");
        Line3D(wallPosition, wallPosition + wallNormal * 50.0f, GFX_RED);
    }
}

void Raycast()
{
    CheckAreaAtFront();
}
} // namespace GOTHIC_NAMESPACE
