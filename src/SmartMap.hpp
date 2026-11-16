namespace GOTHIC_NAMESPACE
{
#if ENGINE == Engine_G2A
void __cdecl sysEvent(void)
{
    XCALL(0x005053E0)
}
#elif ENGINE == Engine_G1
void __cdecl sysEvent(void)
{
    XCALL(0x004F6AC0)
}
#else
void __cdecl sysEvent(void)
{
}
#endif

inline void AddNpcToMap(std::unordered_map<std::string, int> &map, std::string npcName)
{
    if (map.find(npcName) == map.end())
        map.insert({npcName, 1});
}

struct zSmartMap
{
    static inline SECTOR Min;
    static inline SECTOR Max;
    static inline SECTOR CurrentSector;

    static inline zCArray<zCVob *> GlobalVobList;
    static inline zCArray<oCNpc *> Humans;
    static inline zCArray<oCNpc *> Orcs;
    static inline zCArray<oCNpc *> Monsters;
    static inline zCArray<oCNpc *> Dragons;
    static inline zCArray<oCItem *> Items;
    static inline zCArray<oCMobContainer *> Chests;
    static inline zCArray<zCWaypoint *> Waypoints;

    static void InitVobList()
    {
        GlobalVobList.EmptyList();
        auto list = ogame->GetGameWorld()->voblist;
        while (list)
        {
            auto vob = list->data;
            if (vob)
            {
                if (vob->type == zVOB_TYPE_ITEM || vob->type == zVOB_TYPE_MOB || vob->type == zVOB_TYPE_WAYPOINT)
                    GlobalVobList.Insert(vob);
                else if (vob->type == zVOB_TYPE_NSC)
                {
                    oCNpc *npc = static_cast<oCNpc *>(vob);

                    if (vob->GetPositionWorld() == 0.0f)
                    {
                        zVEC3 routPosition = npc->state.GetAIStatePosition();
                        npc->SetPositionWorld(routPosition);
                    }

                    GlobalVobList.Insert(vob);
                }
            }

            list = list->next;
        }

        UpdateInterests();
    }

    static void UpdateInterests()
    {
        Humans.EmptyList();
        Orcs.EmptyList();
        Monsters.EmptyList();
        Dragons.EmptyList();
        Items.EmptyList();
        Chests.EmptyList();
        Waypoints.EmptyList();

        for (int i = 0, end = GlobalVobList.GetNumInList(); i < end; ++i)
        {
            zCVob *vob = GlobalVobList[i];

            if (vob->type == zVOB_TYPE_NSC)
            {
                oCNpc *npc = static_cast<oCNpc *>(vob);

                // Humans
                if (npc->IsHuman())
                    Humans.Insert(npc);

                // Orcs
                else if (npc->IsOrc())
                    Orcs.Insert(npc);

                // Dragons (Gothic 2 only)
#if ENGINE >= Engine_G2
                else if (npc->guild == NPC_GIL_DRAGON)
                    Dragons.Insert(npc);
#endif
                // Monsters
                else
                    Monsters.Insert(npc);
            }

            // Items
            else if (vob->type == zVOB_TYPE_ITEM)
            {
                Items.Insert(static_cast<oCItem *>(vob));
            }

            // Chests
            else if (vob->type == zVOB_TYPE_MOB)
            {
                oCMobContainer *chest = vob->CastTo<oCMobContainer>();
                if (chest && chest->containList.GetNumInList() > 0)
                    Chests.Insert(chest);
            }
        }

        // Waypoints
        auto wp = ogame->GetGameWorld()->wayNet->wplist.next;
        while (wp)
        {
            Waypoints.Insert(wp->data);
            wp = wp->next;
        }
    }

    static void UpdateWorldBounds()
    {
        zCWorld *world = ogame->GetGameWorld();

        auto bbox3D = world->GetBspTree()->bspRoot->bbox3D;
        Min.x = SectorCoord(bbox3D.mins[VX]);
        Min.y = SectorCoord(bbox3D.mins[VZ]);
        Max.x = SectorCoord(bbox3D.maxs[VX]);
        Max.y = SectorCoord(bbox3D.maxs[VZ]);

        UpdateCurrentSector();
    }

    static void UpdateCurrentSector()
    {
        CurrentSector = SectorOf(player->GetPositionWorld());
    }

    // Set by SPACE: the map closes and the navigator starts walking to this sector.
    static inline bool NavigationRequested = false;

    static void Open()
    {
        InitVobList();
        UpdateWorldBounds();
        NavigationRequested = false;

        PlaceIndex = -1;
        Speech::Read(Loc::Format(L"Sector {} {}. {}Press ENTER to hear details. Use arrows to move on the map, PAGE UP "
                                 L"and PAGE DOWN to jump between places. Press SPACE to enable navigator.",
                                 L"Sektor {} {}. {}Enter: szczegóły. Strzałki: ruch po mapie, Page Up i Page Down: "
                                 L"kolejne miejsca. Spacja: nawigacja.",
                                 CurrentSector.x, CurrentSector.y, PlacePrefix(CurrentSector)));

        while (true)
        {
            zinput->ProcessInputEvents();

            if (zinput->KeyToggled(KEY_ESCAPE) || zinput->KeyToggled(KEY_M))
                break;

            if (zinput->KeyToggled(KEY_SPACE))
            {
                NavigationRequested = true;
                break;
            }

            HandleEvent();

            zrenderer->BeginFrame();

            // Render world and elements on the screen
            // Helpful for ppl that can see the game
            RenderWorld();
            screen->Render();

            zrenderer->EndFrame();
            zrenderer->Vid_Blit(1, 0, 0);

            zsound->DoSoundUpdate();
            sysEvent();
        }

        // Do not let the key that closed the map reach the game (SPACE draws the weapon, ESCAPE opens the menu).
        WaitForKeyRelease(KEY_SPACE);
        WaitForKeyRelease(KEY_ESCAPE);
        zinput->ClearKeyBuffer();

        if (NavigationRequested)
            Navigator::StartToSector(CurrentSector);
    }

    static void WaitForKeyRelease(const int key)
    {
        const ULONGLONG deadline = GetTickCount64() + 2000;
        while (zinput->KeyPressed(key) && GetTickCount64() < deadline)
        {
            zinput->ProcessInputEvents();
            sysEvent();
            Sleep(10);
        }
    }

    static void Move(const POINT &at)
    {
        bool stop = false;
        if (at.x != 0)
        {
            if (CurrentSector.x + at.x < Min.x)
            {
                CurrentSector.x = Min.x;
                stop = true;
            }
            else if (CurrentSector.x + at.x > Max.x)
            {
                CurrentSector.x = Max.x;
                stop = true;
            }
            else
                CurrentSector.x += at.x;
        }
        else if (at.y != 0)
        {
            if (CurrentSector.y + at.y < Min.y)
            {
                CurrentSector.y = Min.y;
                stop = true;
            }
            else if (CurrentSector.y + at.y > Max.y)
            {
                CurrentSector.y = Max.y;
                stop = true;
            }
            else
                CurrentSector.y += at.y;
        }

        const std::wstring places = Places::NamesInSector(CurrentSector);
        std::wstring text;
        if (stop)
            text = Loc::Format(L"You can't go further, {} {}", L"Dalej się nie da, {} {}", CurrentSector.x,
                               CurrentSector.y);
        else
            text = std::format(L"{} {}", CurrentSector.x, CurrentSector.y);
        if (!places.empty())
            text += L", " + places;
        Speech::Read(text);
    }

    // "Stary Obóz, zamek. " before the rest of a message, or nothing.
    static std::wstring PlacePrefix(const SECTOR &sector)
    {
        const std::wstring places = Places::NamesInSector(sector);
        return places.empty() ? std::wstring() : places + L". ";
    }

    // PAGE DOWN / PAGE UP walk through the named places in alphabetical order.
    static inline int PlaceIndex = -1;

    static void JumpToPlace(const int direction)
    {
        const std::vector<const Places::Entry *> places = Places::Sorted();
        if (places.empty())
        {
            Speech::Read(Loc::T(L"No named places in this world.", L"W tym świecie nie ma nazwanych miejsc."));
            return;
        }

        const int count = static_cast<int>(places.size());
        if (PlaceIndex < 0)
            PlaceIndex = direction > 0 ? 0 : count - 1;
        else
            PlaceIndex = (PlaceIndex + direction + count) % count;

        const Places::Entry *place = places[PlaceIndex];
        CurrentSector = place->Sector;
        Speech::Read(Loc::Format(L"{}, sector {} {}", L"{}, sektor {} {}", Places::Name(*place), CurrentSector.x,
                                 CurrentSector.y));
    }

    static void Move(const int &x, const int &y)
    {
        POINT point;
        point.x = x;
        point.y = y;

        Move(point);
    }

    static void HandleEvent()
    {
        if (zinput->KeyToggled(KEY_LEFT))
            Move(-1, 0);
        else if (zinput->KeyToggled(KEY_RIGHT))
            Move(+1, 0);
        else if (zinput->KeyToggled(KEY_UP))
            Move(0, +1);
        else if (zinput->KeyToggled(KEY_DOWN))
            Move(0, -1);
        else if (zinput->KeyToggled(KEY_RETURN))
            Details();
        else if (zinput->KeyToggled(KEY_NEXT))
            JumpToPlace(+1);
        else if (zinput->KeyToggled(KEY_PRIOR))
            JumpToPlace(-1);
    }

    static bool IsInCurrentSector(const zVEC3 &position)
    {
        return SameSector(SectorOf(position), CurrentSector);
    }

    static void Details()
    {
        // Results
        std::unordered_map<std::string, int> npcMapList;
        int monsters = 0;
        int orcs = 0;
        bool dragons = false;
        int waypoints = 0;
        int others = 0;

        // Get detalis
        oCWorld *world = ogame->GetGameWorld();
        for (int i = 0, end = GlobalVobList.GetNumInList(); i < end; i++)
        {
            zCVob *vob = GlobalVobList[i];

            if (IsInCurrentSector(vob->GetPositionWorld()))
            {
                if (vob->type == zVOB_TYPE_NSC)
                {
                    oCNpc *npc = static_cast<oCNpc *>(vob);
                    if (npc->IsHuman())
                    {
                        zSTRING npcName = npc->name[0];
                        AddNpcToMap(npcMapList, npcName.ToChar());
                    }
                    else if (npc->IsOrc())
                    {
                        orcs++;
                    }
#if ENGINE >= Engine_G2A
                    else if (npc->guild == NPC_GIL_DRAGON)
                    {
                        dragons = true;
                    }
#endif
                    else
                    {
                        monsters++;
                    }
                }
            }
        }

        // Way net waypoints and the navigator's own patch nodes.
        if (zCWayNet *wayNet = Navigator::GetWayNet())
            waypoints = static_cast<int>(Navigator::WaypointsInSector(wayNet, CurrentSector).size());

        // Messages
        std::wstring message = PlacePrefix(CurrentSector);
        if (waypoints == 0)
        {
            Speech::Read(message + Loc::T(L"Unreachable area.", L"Obszar niedostępny."));
            return;
        }

        message += Loc::T(L"Characters: ", L"Postacie: ");
        if (npcMapList.size() > 0)
        {
            for (auto &pair : npcMapList)
            {
                // if (pair.GetValue() == 1) message += std::format(L"{}, ", pair.GetKey());
                // else others++;
            }

            if (others)
                message += Loc::Format(L"and {} others. ", L"i {} innych. ", others);
        }
        else
            message += Loc::T(L"not detected. ", L"brak. ");

        if (orcs > 0)
            message += Loc::Format(L"Orcs: {}. ", L"Orkowie: {}. ", orcs);
        if (monsters > 0)
            message += Loc::Format(L"Monsters: {}. ", L"Potwory: {}. ", monsters);
        if (dragons)
            message += Loc::T(L"Dragons: not detected! ", L"Smoki! ");

        // Reading
        Speech::Read(message);
    }

    static void RenderWorld()
    {
        if (GlobalVobList.IsEmpty())
            InitVobList();

        zVEC3 a(static_cast<float>(CurrentSector.x * zMAP_SECTOR_SIZE), +6000.0f,
                static_cast<float>(CurrentSector.y * zMAP_SECTOR_SIZE));
        zVEC3 b(a[VX] + zMAP_SECTOR_SIZE, -4000.0f, a[VZ] + zMAP_SECTOR_SIZE);

        ogame->GetCameraVob()->SetPositionWorld(a);
        ogame->GetCameraVob()->SetHeadingWorld(b);

        zlineCache->Line3D(zVEC3(a[VX], 0.0f, a[VZ]), zVEC3(b[VX], 0.0f, a[VZ]), GFX_GREEN, 0);
        zlineCache->Line3D(zVEC3(b[VX], 0.0f, a[VZ]), zVEC3(b[VX], 0.0f, b[VZ]), GFX_GREEN, 0);
        zlineCache->Line3D(zVEC3(b[VX], 0.0f, b[VZ]), zVEC3(a[VX], 0.0f, b[VZ]), GFX_GREEN, 0);
        zlineCache->Line3D(zVEC3(a[VX], 0.0f, b[VZ]), zVEC3(a[VX], 0.0f, a[VZ]), GFX_GREEN, 0);

        for (int i = 0, end = GlobalVobList.GetNumInList(); i < end; ++i)
        {
            zCVob *vob = GlobalVobList[i];

            if (IsInCurrentSector(vob->GetPositionWorld()))
            {
                if (vob->type == zVOB_TYPE_NSC)
                {
                    zVEC3 pos = vob->GetPositionWorld();
                    zlineCache->Line3D(pos, pos + zVEC3(0.0f, 250.0f, 0.0f), GFX_RED, 0);
                }
                if (vob->type == zVOB_TYPE_ITEM)
                {
                    zVEC3 pos = vob->GetPositionWorld();
                    zlineCache->Line3D(pos, pos + zVEC3(0.0f, 250.0f, 0.0f), GFX_YELLOW, 0);
                }
                if (vob->type == zVOB_TYPE_MOB)
                {
                    zVEC3 pos = vob->GetPositionWorld();
                    zlineCache->Line3D(pos, pos + zVEC3(0.0f, 250.0f, 0.0f), GFX_GREEN, 0);
                }
                if (vob->type == zVOB_TYPE_WAYPOINT)
                {
                    zVEC3 pos = vob->GetPositionWorld();
                    zlineCache->Line3D(pos, pos + zVEC3(0.0f, 250.0f, 0.0f), GFX_CYAN, 0);
                }
            }
        }

        screen->PrintCXY(std::format("sector  {}  {}", CurrentSector.x, CurrentSector.y).c_str());

        zCCamera *camera = ogame->GetCamera();
        ogame->GetGameWorld()->Render(*camera);
    }
};
} // namespace GOTHIC_NAMESPACE