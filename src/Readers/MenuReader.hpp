namespace GOTHIC_NAMESPACE
{
namespace Speech
{
void Read(zCMenuItem *item)
{
    if (!item)
        return;

    const zSTRING itemName = item->GetText(0);
    const zSTRING itemLabel = item->GetText(1);

    if (!itemName.IsEmpty() && !itemLabel.IsEmpty())
    {
        ReadFormat("{}: {}", itemName.ToChar(), itemLabel.ToChar());
        return;
    }

    if (!itemName.IsEmpty())
    {
        Read(itemName);
        return;
    }

    if (!itemLabel.IsEmpty())
    {
        Read(itemLabel);
        return;
    }

    Read(item->GetName());
}
} // namespace Speech
} // namespace GOTHIC_NAMESPACE
