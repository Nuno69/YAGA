namespace GOTHIC_NAMESPACE {
	void GameReaderLoop()
	{
		if (zinput->KeyToggled(KEY_H))
			Speech::ReadFormat("{} / {} health", player->GetAttribute(0), player->GetAttribute(1));
	}
}