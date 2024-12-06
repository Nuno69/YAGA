// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	struct zNavPoly {

	};

	struct zNavGroup {
		Array<zNavPoly*> NavPolys;
	};

	class zNavMesh {
		Array<zNavGroup*> NavGroups;
	public:
		void Generate();
	};
}