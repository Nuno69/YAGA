// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
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
}