// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	void CompassReaderLoop() {
		if( zKeyToggled( KEY_F1 ) ) {
			int compassAngle = GetCompassAngle();
			wstring message = wstring::Combine( L"%i degree", compassAngle );
			Read( message );
		}
	}
}