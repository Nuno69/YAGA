// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
#if ENGINE == Engine_G2A
	void __cdecl sysEvent(void) { XCALL(0x005053E0) }
#elif ENGINE == Engine_G1
	void __cdecl sysEvent(void) { XCALL(0x004F6AC0) }
#else
	void __cdecl sysEvent(void) { }
#endif

	SECTOR zSmartMap::Min;
	SECTOR zSmartMap::Max;
	SECTOR zSmartMap::CurrentSector;
	Array<zCVob*> zSmartMap::GlobalVobList;
	Array<oCNpc*> zSmartMap::Humans;
	Array<oCNpc*> zSmartMap::Orcs;
	Array<oCNpc*> zSmartMap::Monsters;
	Array<oCNpc*> zSmartMap::Dragons;
	Array<oCItem*> zSmartMap::Items;
	Array<oCMobContainer*> zSmartMap::Chests;
	Array<zCWaypoint*> zSmartMap::Waypoints;


	const wtext Msg_SmartMapEntry[TSystemLangID::Lang_Max] = {
	  L"Сектор %i %i. Нажмите ЭНТЕР чтобы прослушать детали. Используйте стрелки для перемещения по карте. Нажмите пробел для включения навигатора." // ru
	  L"Sector %i %i. Press ENTER to hear details. Use arrows to move on the map. Press SPACE to enable navigator." // en
	  L"Sector %i %i. Press ENTER to hear details. Use arrows to move on the map. Press SPACE to enable navigator." // de
	  L"Sector %i %i. Press ENTER to hear details. Use arrows to move on the map. Press SPACE to enable navigator." // po
	  L"Sector %i %i. Press ENTER to hear details. Use arrows to move on the map. Press SPACE to enable navigator." // ro
	  L"Sector %i %i. Press ENTER to hear details. Use arrows to move on the map. Press SPACE to enable navigator." // it
	  L"Sector %i %i. Press ENTER to hear details. Use arrows to move on the map. Press SPACE to enable navigator." // sz
	  L"Sector %i %i. Press ENTER to hear details. Use arrows to move on the map. Press SPACE to enable navigator." // es
	};

	const wtext GetLocalisedString(const wtext* stringArray) {
		return stringArray[Union.GetSystemLanguage()];
	}


	inline void GetWorldSectorBounds(SECTOR& min, SECTOR& max) {
		min.x = -500;
		min.y = -500;
		max.x = +500;
		max.y = +500;
	}


	void zSmartMap::InitVobList() {
		GlobalVobList.Clear();
		auto list = ogame->GetGameWorld()->voblist;
		while (list) {
			auto vob = list->data;
			if (vob) {
				if (vob->type == zVOB_TYPE_ITEM || vob->type == zVOB_TYPE_MOB || vob->type == zVOB_TYPE_WAYPOINT) {
					GlobalVobList.Insert(vob);
				}
				else if (vob->type == zVOB_TYPE_NSC) {
					oCNpc* npc = static_cast<oCNpc*>(vob);
					if (vob->GetPositionWorld() == 0.0f) {
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



	void zSmartMap::UpdateInterests() {
		Humans.Clear();
		Orcs.Clear();
		Monsters.Clear();
		Dragons.Clear();
		Items.Clear();
		Chests.Clear();
		Waypoints.Clear();

		for each (zCVob* vob in GlobalVobList) {
			if (vob->type == zVOB_TYPE_NSC) {
				oCNpc* npc = static_cast<oCNpc*>(vob);

				// Humans
				if (npc->IsHuman()) Humans.Insert(npc);

				// Orcs
				else if (npc->IsOrc()) Orcs.Insert(npc);

				// Dragons (Gothic 2 only)
#if ENGINE >= Engine_G2
				else if (npc->guild == NPC_GIL_DRAGON) Dragons.Insert(npc);
#endif
				// Monsters
				else Monsters.Insert(npc);
			}

			// Items
			else if (vob->type == zVOB_TYPE_ITEM) {
				Items.Insert(static_cast<oCItem*>(vob));
			}

			// Chests
			else if (vob->type == zVOB_TYPE_MOB) {
				oCMobContainer* chest = vob->CastTo<oCMobContainer>();
				if (chest && chest->containList.GetNumInList() > 0)
					Chests.Insert(chest);
			}
		}

		// Waypoints
		auto wp = ogame->GetGameWorld()->wayNet->wplist.next;
		while (wp) {
			Waypoints.Insert(wp->data);
			wp = wp->next;
		}
	}


	void zSmartMap::UpdateWorldBounds() {
		zCWorld* world = ogame->GetGameWorld();
		auto bbox3D = world->GetBspTree()->bspRoot->bbox3D;
		Min.x = static_cast<LONG>(bbox3D.mins[VX]) / zMAP_SECTOR_SIZE;
		Min.y = static_cast<LONG>(bbox3D.mins[VZ]) / zMAP_SECTOR_SIZE;
		Max.x = static_cast<LONG>(bbox3D.maxs[VX]) / zMAP_SECTOR_SIZE + 1;
		Max.y = static_cast<LONG>(bbox3D.maxs[VZ]) / zMAP_SECTOR_SIZE + 1;
		UpdateCurrentSector();
	}


	void zSmartMap::UpdateCurrentSector() {
		zVEC3 position = player->GetPositionWorld();
		CurrentSector.x = static_cast<LONG>(position[VX]) / zMAP_SECTOR_SIZE;
		CurrentSector.y = static_cast<LONG>(position[VZ]) / zMAP_SECTOR_SIZE;
	}


	void zSmartMap::Move(const POINT& at) {
		bool stop = false;
		if (at.x != 0) {
			if (CurrentSector.x + at.x < Min.x) {
				CurrentSector.x = Min.x;
				stop = true;
			}
			else if (CurrentSector.x + at.x > Max.x) {
				CurrentSector.x = Max.x;
				stop = true;
			}
			else
				CurrentSector.x += at.x;
		}
		else if (at.y != 0) {
			if (CurrentSector.y + at.y < Min.y) {
				CurrentSector.y = Min.y;
				stop = true;
			}
			else if (CurrentSector.y + at.y > Max.y) {
				CurrentSector.y = Max.y;
				stop = true;
			}
			else
				CurrentSector.y += at.y;
		}

		if (stop)
			Read(wstring::Combine(L"You can't go further, %i %i", CurrentSector.x, CurrentSector.y));
		else
			Read(wstring::Combine(L"%i %i", CurrentSector.x, CurrentSector.y));
	}


	void zSmartMap::Move(const int& x, const int& y) {
		POINT point;
		point.x = x;
		point.y = y;
		Move(point);
	}


	void zSmartMap::Open() {
		InitVobList();
		UpdateWorldBounds();

		wstring messageEntry = wstring::Combine(L"Sector %i %i. Press ENTER to hear details. Use arrows to move on the map. Press SPACE to enable navigator.", CurrentSector.x, CurrentSector.y);
		Read(messageEntry);

		while (true) {
			zinput->ProcessInputEvents();

			if (zKeyToggled(KEY_ESCAPE) || zKeyToggled(KEY_M)) {
				ogame->GetCameraVob()->SetSleeping(False); // DELETE ME
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


	inline void AddNpcToMap(Map<string, int>& map, const string& npcName) {
		auto& pair = map[npcName];
		if (pair.IsNull()) map.Insert(npcName, 1);
		else pair += 1;
	}


	void zSmartMap::Details() {
		// Results
		Map<string, int> npcMapList;
		int monsters = 0;
		int orcs = 0;
		bool dragons = false;
		int waypoints = 0;
		int others = 0;

		// Get detalis
		oCWorld* world = ogame->GetGameWorld();
		for each (zCVob * vob in GlobalVobList) {
			if (IsInCurrentSector(vob->GetPositionWorld())) {
				if (vob->type == zVOB_TYPE_NSC) {
					oCNpc* npc = static_cast<oCNpc*>(vob);
					if (npc->IsHuman()) {
						string npcName = npc->name[0];
						AddNpcToMap(npcMapList, npcName);
					}
					else if (npc->IsOrc()) {
						orcs++;
					}
#if ENGINE >= Engine_G2A
					else if (npc->guild == NPC_GIL_DRAGON) {
						dragons = true;
					}
#endif
					else {
						monsters++;
					}
				}
			}
		}

		for each (zCWaypoint * wp in Waypoints)
			if (IsInCurrentSector(wp->pos))
				waypoints++;

		// Messages
		if (waypoints == 0) {
			Read(L"Unreachable area.");
			return;
		}

		wstring message;
		message += L"Characters: ";
		auto& npcArray = npcMapList.GetArray();
		if (npcArray.GetNum() > 0) {
			for each (auto & pair in npcArray) {
				if (pair.GetValue() == 1) message += wstring::Combine(L"%s, ", pair.GetKey().AToW());
				else others++;
			}

			if (others)
				message += wstring::Combine(L"and %i others. ", others);
		}
		else
			message += L"not detected. ";

		if (orcs > 0)     message += wstring::Combine(L"Orcs: %i. ", orcs);
		if (monsters > 0) message += wstring::Combine(L"Monsters: %i. ", monsters);
		if (dragons)      message += L"Dragons: not detected! ";

		// Reading
		Read(message);
	}


	bool zSmartMap::IsInCurrentSector(const zVEC3& position) {
		return
			int(position[VX] / zMAP_SECTOR_SIZE) == CurrentSector.x &&
			int(position[VZ] / zMAP_SECTOR_SIZE) == CurrentSector.y;
	}


	void zSmartMap::HandleEvent() {
		if (zKeyToggled(KEY_LEFT))			Move(-1, 0);
		else if (zKeyToggled(KEY_RIGHT))	Move(+1, 0);
		else if (zKeyToggled(KEY_UP))		Move(0, +1);
		else if (zKeyToggled(KEY_DOWN))		Move(0, -1);
		else if (zKeyToggled(KEY_RETURN))	Details();

	}


	void zSmartMap::RenderWorld() {
		if (GlobalVobList.IsEmpty())
			InitVobList();

		zVEC3 a(static_cast<float>(CurrentSector.x * zMAP_SECTOR_SIZE), +6000.0f, static_cast<float>(CurrentSector.y * zMAP_SECTOR_SIZE));
		zVEC3 b(a[VX] + zMAP_SECTOR_SIZE, -4000.0f, a[VZ] + zMAP_SECTOR_SIZE);

		ogame->GetCameraVob()->SetSleeping(True);
		ogame->GetCameraVob()->SetPositionWorld(a);
		ogame->GetCameraVob()->SetHeadingWorld(b);

		zlineCache->Line3D(zVEC3(a[VX], 0.0f, a[VZ]), zVEC3(b[VX], 0.0f, a[VZ]), GFX_GREEN, 0);
		zlineCache->Line3D(zVEC3(b[VX], 0.0f, a[VZ]), zVEC3(b[VX], 0.0f, b[VZ]), GFX_GREEN, 0);
		zlineCache->Line3D(zVEC3(b[VX], 0.0f, b[VZ]), zVEC3(a[VX], 0.0f, b[VZ]), GFX_GREEN, 0);
		zlineCache->Line3D(zVEC3(a[VX], 0.0f, b[VZ]), zVEC3(a[VX], 0.0f, a[VZ]), GFX_GREEN, 0);

		for each (zCVob* vob in GlobalVobList) {
			if (IsInCurrentSector(vob->GetPositionWorld())) {
				if (vob->type == zVOB_TYPE_NSC) {
					zVEC3 pos = vob->GetPositionWorld();
					zlineCache->Line3D(pos, pos + zVEC3(0.0f, 250.0f, 0.0f), GFX_RED, 0);
				}
				if (vob->type == zVOB_TYPE_ITEM) {
					zVEC3 pos = vob->GetPositionWorld();
					zlineCache->Line3D(pos, pos + zVEC3(0.0f, 250.0f, 0.0f), GFX_YELLOW, 0);
				}
				if (vob->type == zVOB_TYPE_MOB) {
					zVEC3 pos = vob->GetPositionWorld();
					zlineCache->Line3D(pos, pos + zVEC3(0.0f, 250.0f, 0.0f), GFX_GREEN, 0);
				}
				if (vob->type == zVOB_TYPE_WAYPOINT) {
					zVEC3 pos = vob->GetPositionWorld();
					zlineCache->Line3D(pos, pos + zVEC3(0.0f, 250.0f, 0.0f), GFX_CYAN, 0);
				}
			}
		}

		screen->PrintCXY(string::Combine("sector  %i  %i", CurrentSector.x, CurrentSector.y));

		zCCamera* camera = ogame->GetCamera();
		ogame->GetGameWorld()->Render(*camera);
	}
}