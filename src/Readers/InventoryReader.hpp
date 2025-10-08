namespace GOTHIC_NAMESPACE {
	void ReadInventoryItem(oCItem* item)
	{
		if (item == nullptr)
			return;

		// TO DO: write some type of function for converting ANSI string to WIDE string
		//string message = string::Combine("%z. ", item->name);

		for (int i = 0; i < 6; i++)
		{
			zSTRING& text = item->text[i];

			if (text.Length() == 0)
				continue;

			int& count = item->count[i];
			//message += count != 0 ?
			//	string::Combine("%z - %i.", text, count) :
			//	string::Combine("%z ", text, count);

			//Read(message.AToW());
		}
	}
}