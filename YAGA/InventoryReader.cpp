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

	// TODO: Works instable. Activate method called multiple times.
	// 
	// HOOK Hook_oCItemContainer_Activate PATCH( &oCItemContainer::Activate, &oCItemContainer::Activate_Union );
	// 
	// void oCItemContainer::Activate_Union() {
	// 	if( LastActivatedInventory == this )
	// 		return;
	// 
	// 	LastActivatedInventory = this;
	// 	if( right ) Read( L"Right container" );
	// 	else Read( L"Left container" );
	// 
	// 	Sleep( 1200 );
	// 	THISCALL( Hook_oCItemContainer_Activate )();
	// }
}