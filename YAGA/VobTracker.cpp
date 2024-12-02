// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	NPCTracker* npcTracker;
	void InitializeVobTracker()
	{
		npcTracker = new NPCTracker();
	}

	void GetNearbyVobs()
	{
		if (npcTracker->GetCurrentNumOfNPC() > 0) npcTracker->ClearList();

		for (int i = 0; i < player->vobList.GetNum(); i++)
		{
			zCVob* vob = player->vobList[i];
			oCNpc* currentNPC = zDYNAMIC_CAST<oCNpc>(vob);
			if (currentNPC) npcTracker->AddNPC(currentNPC);
		}
	}

	void VobTrackerLoop()
	{
		GetNearbyVobs();
		npcTracker->SortNPCByDistanceFromPlayer();
	}
}