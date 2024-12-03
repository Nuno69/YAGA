// Supported with union (c) 2020 Union team

// User API for oCNpc
// Add your methods here

void Disable_Union();
void SetWalkStopChasm_Union( int );
int EV_Strafe_Union( oCMsgMovement* );
int FreeLineOfSight_Union( zVEC3& pos, zCVob* vob );

#if ENGINE == Engine_G2
int IsDead() { return attribute[NPC_ATR_HITPOINTS] <= 0; }
#endif