namespace GOTHIC_NAMESPACE
{
void GameReaderLoop()
{
    if (zinput->KeyToggled(KEY_H))
        Speech::Read(Loc::Format(L"{} / {} health", L"{} na {} zdrowia", player->GetAttribute(0),
                                 player->GetAttribute(1)));
}
} // namespace GOTHIC_NAMESPACE