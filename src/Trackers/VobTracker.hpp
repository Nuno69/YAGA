namespace GOTHIC_NAMESPACE {
	NPCTracker* npcTracker;

	void InitializeVobTracker()
	{
		npcTracker = new NPCTracker();
	}

	void GetNearbyVobs()
	{
		if (npcTracker->GetCurrentNumOfNPC() > 0)
		    npcTracker->ClearList();

		for (int i = 0, end = player->vobList.GetNumInList(); i < end; ++i)
		{
			zCVob* vob = player->vobList[i];
			oCNpc* currentNPC = zDYNAMIC_CAST<oCNpc>(vob);

			if (currentNPC)
			    npcTracker->AddNPC(currentNPC);
		}
	}

	void VobTrackerLoop()
	{
		GetNearbyVobs();
		npcTracker->SortNPCByDistanceFromPlayer();
	}
}