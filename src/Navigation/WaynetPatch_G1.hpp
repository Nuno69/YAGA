namespace GOTHIC_NAMESPACE
{
namespace WaynetPatch
{
// Navigator-only waypoints for the Gothic 1 colony (format: WaynetPatch.hpp). Every path here was walked by the
// navigator in game before it was added.
inline const char *CompiledPatch()
{
#if ENGINE == Engine_G1
    return R"(
world WORLD.ZEN
)";
#else
    return "";
#endif
}
} // namespace WaynetPatch
} // namespace GOTHIC_NAMESPACE
