#include "Widget_WorldIndicator.h"

#include "Components/Image.h"

void UWidget_WorldIndicator::UpdateWorldIcon(EWorldState NewWorld)
{
    const int32 IconIndex = static_cast<int32>(NewWorld);

    if (WorldImage && WorldIcons.IsValidIndex(IconIndex))
    {
        if (UTexture2D* Texture = WorldIcons[IconIndex])
        {
            WorldImage->SetBrushFromTexture(Texture, true);
        }
    }

    if (WorldImage)
    {
        if (const FString* const WorldName = WorldNames.Find(NewWorld))
        {
            WorldImage->SetToolTipText(FText::FromString(*WorldName));
        }
        else
        {
            WorldImage->SetToolTipText(FText::GetEmpty());
        }
    }
}
