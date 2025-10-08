namespace Settings
{
	inline int ObjectTrackerReadIndex;
	inline int ObjectTrackerReadDist;
}

#ifdef GOTHIC_NAMESPACE

namespace GOTHIC_NAMESPACE {
	void InitializeObjectTracker()
	{
		Settings::ObjectTrackerReadIndex = zoptions->ReadInt("YAGA", "ObjectTrackerReadIndex", 1);
		Settings::ObjectTrackerReadDist = zoptions->ReadInt("YAGA", "ObjectTrackerReadDist", 1);
	}

	class ObjectTracker
	{
		private:
			oCMobInter* selectedObject;
			oCNpc *selectedNPC;
			std::vector<oCMobInter*> nearbyObjects;
			std::vector<oCNpc*> nearbyNPC;
			int numberOfNPC = 0;
			int numberOfObjects = 0;

		public:
			int objectCursor = 0;
			int npcCursor = 0;

		public:
			int MoveToPrevious(int what);
			int MoveToNext(int what);
			int SelectAtCursor(int what);
			int AddObject(oCMobInter* obj);
			int AddNPC(oCNpc *npc);
			int GetCurrentNumOf(int what);
			int GetHumanizedCursorPosition();
			void ClearList();
			void UpdateObjectCount();
			void SortObjectsByDistanceFromPlayer();
			oCMobInter* GetSelectedObject();
			oCNpc* GetSelectedNPC();
	};
}

#endif