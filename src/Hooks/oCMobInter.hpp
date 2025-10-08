namespace GOTHIC_NAMESPACE {
	// Makes the game more benevolent to interaction accuracy, e.g. allows to interact with an NPC even if the character is not facing it.
	auto Hook_oCMobInter_SearchFreePosition = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0071DFC0, 0x0071DFC0, 0x0071DFC0, 0x0071DFC0)), &oCMobInter::SearchFreePosition_Union, Union::HookType::Hook_CallPatch);
#if ENGINE >= Engine_G2
	TMobOptPos* oCMobInter::SearchFreePosition_Union(oCNpc* npc, float distance)
#else
	TMobOptPos* oCMobInter::SearchFreePosition_Union(oCNpc* npc)
#endif
	{
		zCList<TMobOptPos>* optList = optimalPosList.GetNextInList();
		zVEC3 npcPosition = npc->GetPositionWorld();
		float bestPosDist = FLT_MAX;
		TMobOptPos* bestPos = nullptr;
		while (optList) {
			TMobOptPos* optPos = optList->GetData();
			optList = optList->GetNextInList();
			if (optPos->npc == npc) return optPos;
			if (optPos->npc != nullptr) continue;
			float dist = optPos->trafo.GetTranslation().Distance(npcPosition);
			if (dist < bestPosDist) { bestPosDist = dist; bestPos = optPos; }
		}

		return bestPos;
	}
}