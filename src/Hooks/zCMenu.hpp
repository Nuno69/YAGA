namespace GOTHIC_NAMESPACE
{
using zCMenuVoidHook = Union::Hook<void (zCMenu::*)()>;

inline zCMenuVoidHook Hook_zCMenu_Enter(nullptr);
inline zCMenuVoidHook Hook_zCMenu_PreviousOption(nullptr);
inline zCMenuVoidHook Hook_zCMenu_NextOption(nullptr);

inline void InitializeMenuHooks()
{
    static bool initialized = false;
    if (initialized)
        return;

    Hook_zCMenu_Enter = Union::CreateHook(reinterpret_cast<void *>(zSwitch(0x004CEB90, 0x004DF420, 0x004D90E0, 0x004DB780)),
                                          &zCMenu::Enter_Union, Union::HookType::Hook_Detours);
    Hook_zCMenu_PreviousOption =
        Union::CreateHook(reinterpret_cast<void *>(zSwitch(0x004CE9D0, 0x004DF260, 0x004D8F20, 0x004DB5C0)),
                          &zCMenu::PreviousOption_Union, Union::HookType::Hook_Detours);
    Hook_zCMenu_NextOption =
        Union::CreateHook(reinterpret_cast<void *>(zSwitch(0x004CEAA0, 0x004DF330, 0x004D8FF0, 0x004DB690)),
                          &zCMenu::NextOption_Union, Union::HookType::Hook_Detours);

    initialized = true;
}

// The focused item of a menu, or null. The engine's GetActiveItem returns m_listItems[m_mainSel] without checking the
// index, and a menu with nothing to select (the character status screen) has no valid one: reading the garbage it
// returned crashed the game.
inline zCMenuItem *FocusedMenuItem(zCMenu *menu)
{
    const int index = menu->m_mainSel;
    if (index < 0 || index >= menu->m_listItems.GetNumInList())
        return nullptr;
    return menu->m_listItems[index];
}

// A menu with nothing to select (the character status screen): read all of its text instead, in the order it is seen
// on the screen. The script lists a section's labels first and their values after them, so the items are put in screen
// order: column by column, top to bottom, and the items of one column on one line (a label and its values) are read
// together.
//
// A column starts at an x where some line begins and where, on another line, an item stands after others: on the
// status screen the talents start at 3500 both after the attributes' values ("Poziom 0 | Broń jednoręczna brak 0%")
// and alone ("Magia brak"). A menu of one item per line has no such x and is read top to bottom.
inline void ReadWholeMenu(zCMenu *menu)
{
    struct Piece
    {
        int x;
        int y;
        std::wstring text;
        int line = 0;
        int column = 0;
    };
    std::vector<Piece> pieces;
    for (int i = 0; i < menu->m_listItems.GetNumInList(); ++i)
    {
        zCMenuItem *item = menu->m_listItems[i];
        if (!item)
            continue;
        const zSTRING text = item->GetText(0);
        if (text.IsEmpty())
            continue;
        pieces.push_back({item->m_parPosX, item->m_parPosY, ToWString(text)});
    }
    if (pieces.empty())
        return;

    // Lines: items less than this far apart vertically (in the 8192-unit virtual screen) stand on one line.
    constexpr int SameLine = 100;
    std::stable_sort(pieces.begin(), pieces.end(),
                     [](const Piece &a, const Piece &b) { return std::tie(a.y, a.x) < std::tie(b.y, b.x); });
    for (size_t i = 1; i < pieces.size(); ++i)
        pieces[i].line = pieces[i - 1].line + (pieces[i].y - pieces[i - 1].y >= SameLine ? 1 : 0);

    std::set<int> lineStarts, afterOthers;
    for (size_t i = 0; i < pieces.size(); ++i)
    {
        if (i == 0 || pieces[i].line != pieces[i - 1].line)
            lineStarts.insert(pieces[i].x);
        else
            afterOthers.insert(pieces[i].x);
    }
    std::vector<int> columnStarts;
    for (const int x : lineStarts)
        if (afterOthers.count(x))
            columnStarts.push_back(x); // sorted, as the set is
    for (Piece &piece : pieces)
        piece.column = static_cast<int>(std::upper_bound(columnStarts.begin(), columnStarts.end(), piece.x) -
                                        columnStarts.begin());

    std::stable_sort(pieces.begin(), pieces.end(), [](const Piece &a, const Piece &b) {
        return std::tie(a.column, a.line, a.x) < std::tie(b.column, b.line, b.x);
    });
    std::wstring said;
    for (size_t i = 0; i < pieces.size(); ++i)
    {
        if (i > 0)
        {
            const bool sameLine = pieces[i].column == pieces[i - 1].column && pieces[i].line == pieces[i - 1].line;
            said += sameLine ? L" " : L", ";
        }
        said += pieces[i].text;
    }
    Speech::Read(said);
}

// Read the initially focused item when a menu opens.
void zCMenu::Enter_Union()
{
    (this->*Hook_zCMenu_Enter)();
    if (zCMenuItem *item = FocusedMenuItem(this))
        Speech::Read(item);
    else
        ReadWholeMenu(this);
}

// Read the item focused by pressing the up arrow key.
void zCMenu::PreviousOption_Union()
{
    (this->*Hook_zCMenu_PreviousOption)();
    Speech::Read(FocusedMenuItem(this));
}

// Read the item focused by pressing the down arrow key.
void zCMenu::NextOption_Union()
{
    (this->*Hook_zCMenu_NextOption)();
    Speech::Read(FocusedMenuItem(this));
}
} // namespace GOTHIC_NAMESPACE
