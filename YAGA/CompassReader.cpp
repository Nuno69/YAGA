// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	void CompassReaderLoop() {
		if (zKeyToggled(KEY_F1)) {
			float compassAngle = GetCompassAngle();
			Read(GetCompassName(compassAngle).AToW());
		}

		if (zKeyPressed(KEY_LSHIFT) || zKeyPressed(KEY_RSHIFT)) {
			if (zKeyToggled(KEY_LEFTARROW)) {
				RotateTowardsCompassAngle(-1);

				float compassAngle = GetCompassAngle();
				Read(GetCompassName(compassAngle).AToW());
			}

			if (zKeyToggled(KEY_RIGHTARROW)) {
				RotateTowardsCompassAngle(1);

				float compassAngle = GetCompassAngle();
				Read(GetCompassName(compassAngle).AToW());
			}
		}
	}

	HOOK Hook_oCAIHuman_PC_Turnings AS(&oCAIHuman::PC_Turnings, &oCAIHuman::PC_Turnings_Union);
	void oCAIHuman::PC_Turnings_Union(zBOOL forceRotation) {
		if (!zKeyPressed(KEY_LSHIFT) && !zKeyPressed(KEY_RSHIFT))
			THISCALL(Hook_oCAIHuman_PC_Turnings)(forceRotation);
	}
}