#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widget_HealthBar.generated.h"

class UProgressBar;

/**
 * Displays the player's remaining health in response to world shift drain events.
 */
UCLASS(Blueprintable)
class GAMEJAM_API UWidget_HealthBar : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Progress bar representing the player's current effort percentage. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UProgressBar* HealthProgress;

    /** Updates the progress bar and color based on the provided effort values. */
    UFUNCTION(BlueprintCallable)
    void UpdateEffort(float NewEffort, float MaxEffort);
};

