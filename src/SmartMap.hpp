#define zMAP_SECTOR_SIZE 5000

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

using SECTOR = POINT;

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
        Min.x = static_cast<LONG>(bbox3D.mins[VX]) / zMAP_SECTOR_SIZE;
        Min.y = static_cast<LONG>(bbox3D.mins[VZ]) / zMAP_SECTOR_SIZE;
        Max.x = static_cast<LONG>(bbox3D.maxs[VX]) / zMAP_SECTOR_SIZE + 1;
        Max.y = static_cast<LONG>(bbox3D.maxs[VZ]) / zMAP_SECTOR_SIZE + 1;

        UpdateCurrentSector();
    }

    static void UpdateCurrentSector()
    {
        zVEC3 position = player->GetPositionWorld();
        CurrentSector.x = static_cast<LONG>(position[VX]) / zMAP_SECTOR_SIZE;
        CurrentSector.y = static_cast<LONG>(position[VZ]) / zMAP_SECTOR_SIZE;
    }

    static void Open()
    {
        InitVobList();
        UpdateWorldBounds();

        Speech::ReadFormat("Sector {} {}. Press ENTER to hear details. use arrows to move on the map. Press SPACE to "
                           "enable navigator.",
                           CurrentSector.x, CurrentSector.y);

        while (true)
        {
            zinput->ProcessInputEvents();

            if (zinput->KeyPressed(KEY_ESCAPE) || zinput->KeyPressed(KEY_M))
            {
                ogame->GetCameraVob()->SetSleeping(FALSE); // DELETE ME
                break;
            }

            HandleEvent();

            zrenderer->BeginFrame();
            RenderWorld(); // DEMETE ME
            screen->Render();
            zrenderer->EndFrame();
            zrenderer->Vid_Blit(1, 0, 0);

            zsound->DoSoundUpdate();
            sysEvent();
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

        if (stop)
            Speech::ReadFormat("You can't go further, {} {}", CurrentSector.x, CurrentSector.y);
        else
            Speech::ReadFormat("{} {}", CurrentSector.x, CurrentSector.y);
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
    }

    static bool IsInCurrentSector(const zVEC3 &position)
    {
        return int(position[VX] / zMAP_SECTOR_SIZE) == CurrentSector.x &&
               int(position[VZ] / zMAP_SECTOR_SIZE) == CurrentSector.y;
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

        for (int i = 0, end = Waypoints.GetNumInList(); i < end; ++i)
        {
            zCWaypoint *wp = Waypoints[i];

            if (IsInCurrentSector(wp->pos))
                waypoints++;
        }

        // Messages
        if (waypoints == 0)
        {
            Speech::Read(L"Unreachable area.");
            return;
        }

        std::wstring message;
        message += L"Characters: ";
        if (npcMapList.size() > 0)
        {
            for (auto &pair : npcMapList)
            {
                // if (pair.GetValue() == 1) message += std::format(L"{}, ", pair.GetKey());
                // else others++;
            }

            if (others)
                message += std::format(L"and {} others. ", others);
        }
        else
            message += L"not detected. ";

        if (orcs > 0)
            message += std::format(L"Orcs: {}. ", orcs);
        if (monsters > 0)
            message += std::format(L"Monsters: {}. ", monsters);
        if (dragons)
            message += L"Dragons: not detected! ";

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

        ogame->GetCameraVob()->SetSleeping(TRUE);
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