#include "Widget_Layout.h"

#include "Widget_WorldIndicator.h"

void UWidget_Layout::NativeConstruct()
{
    Super::NativeConstruct();

    if (!WorldWidgetInstance && WorldWidgetClass)
    {
        WorldWidgetInstance = CreateWidget<UWidget_WorldIndicator>(GetWorld(), WorldWidgetClass);
        if (WorldWidgetInstance)
        {
            WorldWidgetInstance->AddToViewport();
        }
    }
}
