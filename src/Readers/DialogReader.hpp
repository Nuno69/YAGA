namespace GOTHIC_NAMESPACE
{
namespace Speech
{
void Read(zCViewDialogChoice *choice)
{
    if (!choice)
        return;

    const zSTRING selectedText = choice->GetSelectedText();
    if (!selectedText.IsEmpty())
        Read(selectedText);
}
} // namespace Speech
} // namespace GOTHIC_NAMESPACE
