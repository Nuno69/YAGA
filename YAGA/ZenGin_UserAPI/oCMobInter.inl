#if ENGINE >= Engine_G2
TMobOptPos* SearchFreePosition_Union( oCNpc* npc, float distance );
#else
TMobOptPos* SearchFreePosition_Union( oCNpc * npc );
#endif