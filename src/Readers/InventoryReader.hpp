namespace GOTHIC_NAMESPACE
{
namespace Speech
{
void Read(oCItem *item)
{
    if (item == nullptr)
        return;

    // "Stary Miecz, 3. Obrażenia: 12. Wymagana siła: 10. Broń jednoręczna. Wartość: 12."
    zSTRING message = item->name;
    if (item->amount > 1)
    {
        message += ", ";
        message += item->amount;
    }
    message += ".";

    for (int i = 0; i < 6; i++)
    {
        const zSTRING &text = item->text[i];
        if (text.Length() == 0)
            continue;

        message += " ";
        message += text;

        const int &count = item->count[i];
        if (count != 0)
        {
            message += " ";
            message += count;
        }

        message += ".";
    }

    Read(message);
}
} // namespace Speech
} // namespace GOTHIC_NAMESPACE