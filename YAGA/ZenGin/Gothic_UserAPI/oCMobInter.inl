// Supported with union (c) 2020 Union team

// User API for oCMobInter
// Add your methods here

#if ENGINE >= Engine_G2
TMobOptPos* SearchFreePosition_Union( oCNpc* npc, float distance );
#else
TMobOptPos* SearchFreePosition_Union( oCNpc * npc );
#endif