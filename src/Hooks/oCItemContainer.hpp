namespace GOTHIC_NAMESPACE
{
// The item whose description was read last, and how many there were. The game draws the description of the selected
// item every frame; reading it every time made the screen reader start over each frame ("sssss...strzała").
inline oCItem *LastReadItem = nullptr;
inline int LastReadAmount = -1;

// Reads the selected item once: when the selection changes, or when the number of that item changes.
auto Hook_oCItemContainer_DrawItemInfo =
    Union::CreateHook(SIGNATURE_OF(&oCItemContainer::DrawItemInfo), &oCItemContainer::DrawItemInfo_Union,
                      Union::HookType::Hook_CallPatch);
void oCItemContainer::DrawItemInfo_Union(oCItem *item, zCWorld *renderWorld)
{
    if (item && (item != LastReadItem || item->amount != LastReadAmount))
    {
        LastReadItem = item;
        LastReadAmount = item->amount;
        Speech::Read(item);
    }
    (this->*Hook_oCItemContainer_DrawItemInfo)(item, renderWorld);
}

// Forgets the item read last, so it is read again when the inventory opens next time.
auto Hook_oCItemContainer_Close = Union::CreateHook(SIGNATURE_OF(&oCItemContainer::Close),
                                                    &oCItemContainer::Close_Union, Union::HookType::Hook_CallPatch);
void oCItemContainer::Close_Union()
{
    LastReadItem = nullptr;
    LastReadAmount = -1;
    Speech::StopReading();
    (this->*Hook_oCItemContainer_Close)();
}

#if ENGINE == Engine_G2A
// Reads container change to the right container.
auto Hook_oCItemContainer_NextItem = Union::CreateHook(SIGNATURE_OF(&oCItemContainer::NextItem),
                                                       &oCItemContainer::NextItem_U, Union::HookType::Hook_Detours);
void oCItemContainer::NextItem_U()
{
    (this->*Hook_oCItemContainer_NextItem)();

    if (!this->IsActive())
        Speech::Read(L"Right container", 0);
}
#endif

// Reads container change to the left container.
auto Hook_oCItemContainer_PrevItem = Union::CreateHook(SIGNATURE_OF(&oCItemContainer::PrevItem),
                                                       &oCItemContainer::PrevItem_U, Union::HookType::Hook_Detours);
void oCItemContainer::PrevItem_U()
{
    (this->*Hook_oCItemContainer_PrevItem)();

    if (!this->IsActive())
        Speech::Read(L"Left container", 0);
}
} // namespace GOTHIC_NAMESPACE