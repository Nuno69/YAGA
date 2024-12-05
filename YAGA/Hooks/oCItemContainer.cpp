// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	// Reads an item once it is drawn.
	HOOK Hook_oCItemContainer_DrawItemInfo PATCH(&oCItemContainer::DrawItemInfo, &oCItemContainer::DrawItemInfo_Union);
	void oCItemContainer::DrawItemInfo_Union(oCItem* item, zCWorld* renderWorld)
	{
		ReadInventoryItem(item);
		THISCALL(Hook_oCItemContainer_DrawItemInfo)(item, renderWorld);
	}

	// Resets all previously read items to null so to read them properly next time.
	HOOK Hook_oCItemContainer_Close PATCH(&oCItemContainer::Close, &oCItemContainer::Close_Union);
	void oCItemContainer::Close_Union()
	{
		ReadInventoryItem(NULL);
		THISCALL(Hook_oCItemContainer_Close)();
	}

#if ENGINE == Engine_G2A
	// Reads container change to the right container.
	HOOK Hook_oCItemContainer_NextItem AS(&oCItemContainer::NextItem, &oCItemContainer::NextItem_U);
	void oCItemContainer::NextItem_U()
	{
		THISCALL(Hook_oCItemContainer_NextItem)();

		if (!this->IsActive())
			Read(L"Right container", 0);
	}
#endif

	// Reads container change to the left container.
	HOOK Hook_oCItemContainer_PrevItem AS(&oCItemContainer::PrevItem, &oCItemContainer::PrevItem_U);
	void oCItemContainer::PrevItem_U()
	{
		THISCALL(Hook_oCItemContainer_PrevItem)();

		if (!this->IsActive())
			Read(L"Left container", 0);
	}
}