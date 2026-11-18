namespace GOTHIC_NAMESPACE
{
namespace Places
{
// Places of the Gothic 1 colony (WORLD.ZEN). Polish names follow the Polish release (quest log and item texts),
// English ones the English release. The waypoint is where SPACE on the map leads: for a place behind a gate or door
// the game opens by script (the monastery, the fog tower, the Sleeper's temple, the castle ruins' moving floor) it is
// the spot in front of it.
inline const char *PlacesWorld()
{
    return "WORLD.ZEN";
}

inline const std::vector<Place> &Table()
{
    static const std::vector<Place> table = {
#if ENGINE == Engine_G1
        {L"Exchange place", L"Miejsce wymiany", "OW_PATH_1_16_8"},
        {L"Abandoned mine", L"Opuszczona kopalnia", "OW_VM_ENTRANCE"},
        {L"Old Camp, main gate", L"Stary Obóz, główna brama", "OCR_MAINGATE_OUTSIDE"},
        {L"Old Camp, marketplace", L"Stary Obóz, targowisko", "OCR_MARKETPLACE_SCAVENGER"},
        {L"Old Camp, arena", L"Stary Obóz, arena", "OCR_ARENABATTLE_OUTSIDE"},
        {L"Old Camp, castle", L"Stary Obóz, zamek", "OCC_CENTER_1"},
        {L"New Camp", L"Nowy Obóz", "NC_ENTRANCE_WP"},
        {L"New Camp, tavern", L"Nowy Obóz, karczma", "NC_TAVERN_IN"},
        {L"Swamp Camp", L"Obóz Bractwa", "PSI_START"},
        {L"Swamp Camp, temple", L"Obóz Bractwa, świątynia", "PSI_TEMPLE_COURT_2"},
        {L"Old Mine", L"Stara Kopalnia", "OW_OM_ENTRANCE04"},
        {L"Free Mine", L"Wolna Kopalnia", "FMC_ENTRANCE"},
        {L"Orc graveyard", L"Cmentarzysko orków", "OW_PATH_198_ORCGRAVEYARD9"},
        {L"Sleeper's temple", L"Świątynia Śniącego", "OW_PATH_06_15"},
        {L"Orc city", L"Miasto orków", "OW_ORC_BOSS3"},
        {L"Mountain fortress", L"Górska forteca", "LOCATION_19_03"},
        {L"Monastery ruins", L"Ruiny klasztoru", "OW_PATH_175_GATE1"},
        {L"Troll canyon", L"Kanion trolli", "LOCATION_12_12"},
        {L"Quentin's camp", L"Obóz Quentina", "LOCATION_11_10"},
        {L"Stone circle", L"Kamienny krąg", "OW_PATH_3_STONES"},
        {L"Fog tower", L"Wieża Mgieł", "FOGTOWER_OUT"},
        {L"Xardas' tower", L"Wieża Xardasa", "LOCATION_03_IN"},
        {L"Sunken tower", L"Zatopiona wieża", "MAGICTOWER_COR"},
        {L"Castle ruins", L"Ruiny zamku", "CASTLE_MOVEMENT_STRAIGHT3"},
#endif
    };
    return table;
}
} // namespace Places
} // namespace GOTHIC_NAMESPACE
