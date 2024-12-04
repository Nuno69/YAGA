// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	static oCItem* LastReadedItem = Null;

	void ReadInventoryItem(oCItem* item)
	{
		if (LastReadedItem != item)
		{
			LastReadedItem = item;
			if (item == Null) return;

			string message = string::Combine("%z. ", item->name);
			for (int i = 0; i < 6; i++) {
				zSTRING& text = item->text[i];
				if (text.Length() == 0) continue;

				int& count = item->count[i];
				message += count != 0 ?
					string::Combine("%z - %i.", text, count) :
					string::Combine("%z ", text, count);

				Read(message.AToW());
			}
		}
	}
}