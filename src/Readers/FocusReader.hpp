namespace GOTHIC_NAMESPACE {
	static inline zCVob* LastFocusedVob = nullptr;
	static inline oCItem* item = nullptr;
	static inline oCNpc* npc = nullptr;
	static inline oCMobContainer* container = nullptr;

	void FocusReaderLoop()
	{
		zCVob* currentVob = player->GetFocusVob();
		if (currentVob == nullptr) { LastFocusedVob = nullptr; }
		// Maybe the vob is an item? Or an NPC? We'll see!
		if (currentVob == LastFocusedVob)
		    return;

        LastFocusedVob = currentVob;
        item = currentVob->CastTo<oCItem>();
        npc = currentVob->CastTo<oCNpc>();
        container = currentVob->CastTo<oCMobContainer>();

        if (item != nullptr) {
            Speech::Read(item->name);
            return;
        }

        if (npc != nullptr) {
            Speech::Read(npc->GetName(0));
            if (npc->IsDead()) {
                Speech::Read(L"Is dead");
            }
            return;
        }

        if (container != nullptr) {
            Speech::Read(container->GetName());
            return;
        }

        Speech::Read(currentVob->GetObjectName());
	}
}