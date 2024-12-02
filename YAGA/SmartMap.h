// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
#define zMAP_SECTOR_SIZE 5000

	typedef POINT SECTOR;

	struct zSmartMap {
		static SECTOR Min;
		static SECTOR Max;
		static SECTOR CurrentSector;

		static Array<zCVob*> GlobalVobList;
		static Array<oCNpc*> Humans;
		static Array<oCNpc*> Orcs;
		static Array<oCNpc*> Monsters;
		static Array<oCNpc*> Dragons;
		static Array<oCItem*> Items;
		static Array<oCMobContainer*> Chests;
		static Array<zCWaypoint*> Waypoints;

		static void InitVobList();
		static void UpdateInterests();
		static void UpdateWorldBounds();
		static void UpdateCurrentSector();
		static void Open();
		static void Move(const POINT& at);
		static void Move(const int& x, const int& y);
		static void HandleEvent();
		static bool IsInCurrentSector(const zVEC3& position);
		static void Details();
		static void RenderWorld();
	};
}