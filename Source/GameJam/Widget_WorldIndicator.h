#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WorldShiftTypes.h"
#include "Widget_WorldIndicator.generated.h"

class UImage;
class UTexture2D;

/**
 * Displays the icon associated with the currently active world state.
 */
UCLASS(Blueprintable)
class GAMEJAM_API UWidget_WorldIndicator : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Textures indexed by the world state enum value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
    TArray<TObjectPtr<UTexture2D>> WorldIcons;

    /** Image widget that renders the selected world texture. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    TObjectPtr<UImage> WorldImage;

    /** Friendly names used as tooltips for each world state. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
    TMap<EWorldState, FString> WorldNames;

    /** Updates the icon and tooltip when the world changes. */
    UFUNCTION(BlueprintCallable)
    void UpdateWorldIcon(EWorldState NewWorld);
};
