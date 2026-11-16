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
# The lake below the intro cliff kills a swimming hero.
avoid WP_INTRO_FALL WP_INTRO_FALL2
avoid WP_INTRO_FALL2 WP_INTRO_SHORE

# The south of the Swamp Camp (49 waypoints) has no way to the rest of the camp.
link PATH_AROUND_PSI08 PSI_GUARD3
link PATH_AROUND_PSI04 PSI_HERB_SHOP_01

# Xardas' tower: the ground floor waypoints start a step past the entrance waypoint.
link DT_E1_01 LOCATION_03_IN

# Path from the New Camp to the Old Mine: the way net dips into the ravine under the bridge, where the hero is
# trapped. Cross on the bridge instead.
link OW_PATH_057 OW_PATH_059
avoid OW_PATH_057 OW_PATH_058
avoid OW_PATH_058 OW_PATH_059

# The same further north, on the bridge over the river before the Old Mine: OW_PATH_149 is in the water under the
# end of the bridge. A node on the bridge above it takes its place.
node YAGA_OM_BRIDGE -31126 513 14036
link OW_PATH_150 YAGA_OM_BRIDGE
link YAGA_OM_BRIDGE OW_PATH_155
avoid OW_PATH_150 OW_PATH_149
avoid OW_PATH_149 OW_PATH_155
)";
#else
    return "";
#endif
}
} // namespace WaynetPatch
} // namespace GOTHIC_NAMESPACE
