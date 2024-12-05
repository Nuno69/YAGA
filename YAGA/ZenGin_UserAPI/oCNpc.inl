void Disable_Union();
void SetWalkStopChasm_Union(int);
int EV_Strafe_Union(oCMsgMovement* msg);
int FreeLineOfSight_Union(zVEC3& pos, zCVob* vob);

#if ENGINE >= Engine_G2
TMobOptPos* SearchFreePosition_Union(oCNpc* npc, float distance);
#else
TMobOptPos* SearchFreePosition_Union(oCNpc * npc);
#endif