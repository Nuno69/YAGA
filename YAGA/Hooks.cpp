// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Prevents the playable character from falling from ledges.
	HOOK Hook_oCNpc_EV_Strafe PATCH(&oCNpc::EV_Strafe, &oCNpc::EV_Strafe_Union);
	int oCNpc::EV_Strafe_Union(oCMsgMovement* msg)
	{
		int ok = THISCALL(Hook_oCNpc_EV_Strafe)(msg);
		if (this == player && anictrl->walkmode == ANI_WALKMODE_RUN)
			SetWalkStopChasm(!ok);

		return ok;
	}

	// Prevents knockback damage dealt by dragons. The damage is still taken but the character doesn't start flying.
	HOOK Hook_oCAIHuman_StartFlyDamage PATCH(&oCAIHuman::StartFlyDamage, &oCAIHuman::StartFlyDamage_Union);
	void oCAIHuman::StartFlyDamage_Union(float damage, zVEC3& flyDir)
	{
		if (this != player->human_ai)
			THISCALL(Hook_oCAIHuman_StartFlyDamage)(damage, flyDir);
	}

	// Makes the game more benevolent to interaction accuracy, e.g. allows to interact with an NPC even if the character is not facing it.
	HOOK Hook_oCMobInter_SearchFreePosition PATCH(&oCMobInter::SearchFreePosition, &oCMobInter::SearchFreePosition_Union);
#if ENGINE >= Engine_G2
	TMobOptPos* oCMobInter::SearchFreePosition_Union(oCNpc* npc, float distance)
#else
	TMobOptPos* oCMobInter::SearchFreePosition_Union(oCNpc* npc)
#endif
	{
		zCList<TMobOptPos>* optList = optimalPosList.GetNextInList();
		zVEC3 npcPosition = npc->GetPositionWorld();
		float bestPosDist = FLT_MAX;
		TMobOptPos* bestPos = Null;
		while (optList) {
			TMobOptPos* optPos = optList->GetData();
			optList = optList->GetNextInList();
			if (optPos->npc == npc) return optPos;
			if (optPos->npc != Null) continue;
			float dist = optPos->trafo.GetTranslation().Distance(npcPosition);
			if (dist < bestPosDist) { bestPosDist = dist; bestPos = optPos; }
		}

		return bestPos;
	}

	// Removes the line of sight obstruction.
	HOOK Hook_oCNpc_FreeLineOfSight PATCH(&oCNpc::FreeLineOfSight, &oCNpc::FreeLineOfSight_Union);
	int oCNpc::FreeLineOfSight_Union(zVEC3& pos, zCVob* vob)
	{
		return TRUE;
	}

	// Disables NPC removal from world, will be used in navigation.
	HOOK Hook_oCNpc_Disable_Union PATCH(&oCNpc::Disable, &oCNpc::Disable_Union);
	void oCNpc::Disable_Union()
	{
		zVEC3 routPosition = state.GetAIStatePosition();
		if (routPosition != 0.0f)
			SetPositionWorld(routPosition);

		THISCALL(Hook_oCNpc_Disable_Union)();
	}

	// Binds sounds to objects that are spawned in the game world.
	HOOK Hook_zCVob_ThisVobAddedToWorld PATCH(&zCVob::ThisVobAddedToWorld, &zCVob::ThisVobAddedToWorld_Union);
	void zCVob::ThisVobAddedToWorld_Union(zCWorld* world)
	{
		THISCALL(Hook_zCVob_ThisVobAddedToWorld)(world);
		if (this != player) {
			if (CastTo<oCMobContainer>()) {
				BindSound3D("WhisperingChest.wav", this);
			}
			if (CastTo<oCItem>()) {
				int handle = BindSound3D("WhisperingItem.wav", this);
			}
			if (CastTo<oCNpc>()) {
				int handle = BindSound3D("WhisperingNPC.wav", this);
				// 	if( handle != Invalid )
				// 		SoundHandles.Insert( handle );
			}
		}
	}

	// Removes the sound associated with the given vob that is deleted from world.
	HOOK Hook_zCVob_ThisVobRemovedFromWorld PATCH(&zCVob::ThisVobRemovedFromWorld, &zCVob::ThisVobRemovedFromWorld_Union);
	void zCVob::ThisVobRemovedFromWorld_Union(zCWorld* world)
	{
		// TODO
		THISCALL(Hook_zCVob_ThisVobRemovedFromWorld)(world);
	}

	// Disables hero turning left/right while holding SHIFT key
	HOOK Hook_oCAIHuman_PC_Turnings AS(&oCAIHuman::PC_Turnings, &oCAIHuman::PC_Turnings_Union);
	void oCAIHuman::PC_Turnings_Union(zBOOL forceRotation)
	{
		if (!zKeyPressed(KEY_LSHIFT) && !zKeyPressed(KEY_RSHIFT))
			THISCALL(Hook_oCAIHuman_PC_Turnings)(forceRotation);
	}

	// Reads the selected dialog choice.
	HOOK Ivk_HighlightSelected_Union PATCH(&zCViewDialogChoice::HighlightSelected, &zCViewDialogChoice::HighlightSelected_Union);
	void zCViewDialogChoice::HighlightSelected_Union()
	{
		THISCALL(Ivk_HighlightSelected_Union)();
		ReadDialogChoice(this);
	}

	// Reads the default dialog choice when a dialog box is presented.
	HOOK Ivk_StartSelection_Union PATCH(&zCViewDialogChoice::StartSelection, &zCViewDialogChoice::StartSelection_Union);
	void zCViewDialogChoice::StartSelection_Union()
	{
		THISCALL(Ivk_StartSelection_Union)();
		ReadDialogChoice(this);
	}

	// Reads an item once it is drawn.
	HOOK Hook_oCItemContainer_DrawItemInfo PATCH(&oCItemContainer::DrawItemInfo, &oCItemContainer::DrawItemInfo_Union);
	void oCItemContainer::DrawItemInfo_Union(oCItem* item, zCWorld* renderWorld)
	{
		ReadInventoryItem(item);
		THISCALL(Hook_oCItemContainer_DrawItemInfo)(item, renderWorld);
	}

	// Resets all previously read items to null so to read them properly next time.
	HOOK Hook_oCItemContainer_Close PATCH(&oCItemContainer::Close, &oCItemContainer::Close_Union);
	void oCItemContainer::Close_Union()
	{
		ReadInventoryItem(NULL);
		THISCALL(Hook_oCItemContainer_Close)();
	}

#if ENGINE == Engine_G2A
	// Reads container change to the right container.
	HOOK Hook_oCItemContainer_NextItem AS(&oCItemContainer::NextItem, &oCItemContainer::NextItem_U);
	void oCItemContainer::NextItem_U()
	{
		THISCALL(Hook_oCItemContainer_NextItem)();

		if (!this->IsActive())
			Read(L"Right container", 0);
	}

	// Reads container change to the left container.
	HOOK Hook_oCItemContainer_PrevItem AS(&oCItemContainer::PrevItem, &oCItemContainer::PrevItem_U);
	void oCItemContainer::PrevItem_U()
	{
		THISCALL(Hook_oCItemContainer_PrevItem)();

		if (!this->IsActive())
			Read(L"Left container", 0);
	}

	// Reads a menu item focused by pressing the up arrow key.
	HOOK Ivk_PreviousOption_Union PATCH(&zCMenu::PreviousOption, &zCMenu::PreviousOption_Union);
	void zCMenu::PreviousOption_Union()
	{
		THISCALL(Ivk_PreviousOption_Union)();
		ReadMenuItem(this->GetActiveItem());
	}
	
	// Reads a menu item focused by pressing the down arrow key.
	HOOK Ivk_NextOption_Union PATCH(&zCMenu::NextOption, &zCMenu::NextOption_Union);
	void zCMenu::NextOption_Union()
	{
		THISCALL(Ivk_NextOption_Union)();
		ReadMenuItem(this->GetActiveItem());
	}
#endif
}