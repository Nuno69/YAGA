// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  static oCItem* LastReadedItem = Null;
	static oCItemContainer* LastActivatedInventory = Null;

	void ReadInventoryItem( oCItem* item ) {
		if( LastReadedItem != item ) {
			LastReadedItem = item;
			if( item == Null ) return;

			string message = string::Combine( "%z. ", item->name );
			for( int i = 0; i < 6; i++ ) {
				zSTRING& text = item->text[i];
				if( text.Length() == 0 ) continue;

				int& count = item->count[i];
				message += count != 0 ?
					string::Combine( "%z - %i.", text, count ) :
					string::Combine( "%z ", text, count );

				Read( message.AToW() );
			}
		}
	}

	HOOK Hook_oCItemContainer_DrawItemInfo PATCH( &oCItemContainer::DrawItemInfo, &oCItemContainer::DrawItemInfo_Union );

	void oCItemContainer::DrawItemInfo_Union( oCItem* item, zCWorld* renderWorld ) {
		ReadInventoryItem( item );
		THISCALL( Hook_oCItemContainer_DrawItemInfo )(item, renderWorld);
	}

	HOOK Hook_oCItemContainer_Close PATCH( &oCItemContainer::Close, &oCItemContainer::Close_Union );

	void oCItemContainer::Close_Union() {
		ReadInventoryItem( Null );
		LastActivatedInventory = Null;
		THISCALL( Hook_oCItemContainer_Close )();
	}
#if ENGINE == Engine_G2A
	HOOK Hook_oCItemContainer_NextItem AS( &oCItemContainer::NextItem, &oCItemContainer::NextItem_U);

	void oCItemContainer::NextItem_U()
	{
		oCViewDialogTrade* this_ = oCInformationManager::GetInformationManager().DlgTrade;

		bool active = this_->DlgInventoryPlayer->Inventory->IsActive();
		THISCALL(Hook_oCItemContainer_NextItem)();
		if (active != this_->DlgInventoryPlayer->Inventory->IsActive())
		{
			Read(L"Right container");
			Sleep(1200);
		}
	}

	HOOK Hook_oCItemContainer_PrevItem AS( &oCItemContainer::PrevItem, &oCItemContainer::PrevItem_U);

	void oCItemContainer::PrevItem_U()
	{
		oCViewDialogTrade* this_ = oCInformationManager::GetInformationManager().DlgTrade;

		bool active = this_->DlgInventoryNpc->StealContainer->IsActive();
		THISCALL(Hook_oCItemContainer_PrevItem)();
		if (active != this_->DlgInventoryNpc->StealContainer->IsActive())
		{
			Read(L"Left container");
			Sleep(1200);
		}
	}
#endif

	// TODO: Previous attempts
/*
	HOOK Hook_oCViewDialogTrade_OnSection AS(&oCViewDialogTrade::OnSection, &oCViewDialogTrade::OnSection_Union);

	void oCViewDialogTrade::OnSection_Union(zETradeDialogSection section)
	{
#if ENGINE == Engine_G2A
		switch (section)
		{
		case Gothic_II_Addon::oCViewDialogTrade::TRADE_SECTION_LEFT_INVENTORY:
			Read(L"Right container");
			Sleep(1200);
			break;
		case Gothic_II_Addon::oCViewDialogTrade::TRADE_SECTION_RIGHT_INVENTORY:
			Read(L"Left container");
			Sleep(1200);
			break;
		default:
			break;
		}
#endif // ENGINE == Engine_G2A
		THISCALL(Hook_oCViewDialogTrade_OnSection)(section);
	}
	*/

	/*
	HOOK Hook_oCViewDialogTrade_HandleEvent AS(&oCViewDialogTrade::HandleEvent, &oCViewDialogTrade::HandleEvent_U);

	int oCViewDialogTrade::HandleEvent_U(int n)
	{
		oCViewDialogTrade* this_ = oCInformationManager::GetInformationManager().DlgTrade;

		int handled = THISCALL(Hook_oCViewDialogTrade_HandleEvent)(n);
		if (this_->DlgInventoryPlayer->Inventory)
			if ((this_->DlgInventoryPlayer->Inventory->IsActive()) &&
				(this_->SectionTrade != TRADE_SECTION_RIGHT_INVENTORY))
			{
				Read(L"Player's container");
				this_->OnSection(TRADE_SECTION_RIGHT_INVENTORY);
				Sleep(1200);
			}
		if (this_->DlgInventoryNpc->StealContainer)
			if ((this_->DlgInventoryNpc->StealContainer->IsActive()) &&
				(this_->SectionTrade != TRADE_SECTION_LEFT_INVENTORY))
			{
				Read(L"NPC's container");
				this_->OnSection(TRADE_SECTION_LEFT_INVENTORY);
				Sleep(1200);
			}
		return handled;
	}
*/
	//TODO: Works instable. Activate method called multiple times.
	/*
	HOOK Hook_oCItemContainer_Activate PATCH( &oCItemContainer::Activate, &oCItemContainer::Activate_Union );
	
	void oCItemContainer::Activate_Union() {
		if( LastActivatedInventory == this )
			return;
	
		LastActivatedInventory = this;
		if( right ) Read( L"Right container" );
		else Read( L"Left container" );
	
		Sleep( 1200 );
		THISCALL( Hook_oCItemContainer_Activate )();
	}
	*/


}