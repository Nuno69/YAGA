namespace GOTHIC_NAMESPACE
{
namespace Speech
{
void Read(oCItem *item)
{
    if (item == nullptr)
        return;

    zSTRING message = item->name;
    message += ". ";

    for (int i = 0; i < 6; i++)
    {
        const zSTRING &text = item->text[i];
        if (text.Length() == 0)
            continue;

        message += text;

        const int &count = item->count[i];
        if (count != 0)
        {
            message += " - ";
            message += count;
        }

        message += ".";
    }

    Read(message);
}
} // namespace Speech
} // namespace GOTHIC_NAMESPACE