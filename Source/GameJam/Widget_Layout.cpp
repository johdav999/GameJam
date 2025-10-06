#include "Widget_Layout.h"

#include "WorldWidget.h"

void UWidget_Layout::NativeConstruct()
{
    Super::NativeConstruct();

    if (!WorldWidgetInstance && WorldWidgetClass)
    {
        WorldWidgetInstance = CreateWidget<UWorldWidget>(GetWorld(), WorldWidgetClass);
        if (WorldWidgetInstance)
        {
            WorldWidgetInstance->AddToViewport();
        }
    }
}
