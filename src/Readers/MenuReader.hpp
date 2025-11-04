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

    ReadFormat("{}: {}", itemName.ToChar(), itemLabel.ToChar());
}
} // namespace Speech
} // namespace GOTHIC_NAMESPACE