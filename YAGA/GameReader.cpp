// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

	// Add your code here . . .

void GameReaderLoop()
{
	if (zKeyToggled(KEY_H)) {
		Read(string::Combine("%i/%i health", player->GetAttribute(0), player->GetAttribute(1)).AToW());
}
}
}