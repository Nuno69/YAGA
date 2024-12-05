void Hook_Disable();
void Hook_SetWalkStopChasm(int stop);
int Patch_FreeLineOfSight(zVEC3& pos, zCVob* vob);

#if ENGINE == Engine_G2
int IsDead() { return attribute[NPC_ATR_HITPOINTS] <= 0; }
#endif