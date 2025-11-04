namespace GOTHIC_NAMESPACE
{
// Reads an item once it is drawn.
auto Hook_oCItemContainer_DrawItemInfo =
    Union::CreateHook(SIGNATURE_OF(&oCItemContainer::DrawItemInfo), &oCItemContainer::DrawItemInfo_Union,
                      Union::HookType::Hook_CallPatch);
void oCItemContainer::DrawItemInfo_Union(oCItem *item, zCWorld *renderWorld)
{
    Speech::Read(item);
    (this->*Hook_oCItemContainer_DrawItemInfo)(item, renderWorld);
}

// Resets all previously read items to null so to read them properly next time.
auto Hook_oCItemContainer_Close = Union::CreateHook(SIGNATURE_OF(&oCItemContainer::Close),
                                                    &oCItemContainer::Close_Union, Union::HookType::Hook_CallPatch);
void oCItemContainer::Close_Union()
{
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