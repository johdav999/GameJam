#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HintTypes.h"
#include "Widget_Layout.generated.h"

class UWidget_HealthBar;
class UWidget_WorldIndicator;
class UWorldWidget;
class UGameJamGameInstance;

/**
 * Root HUD layout widget that exposes references to key UI elements for blueprint wiring.
 */
UCLASS(Blueprintable)
class GAMEJAM_API UWidget_Layout : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Visual indicator for the currently active world. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UWidget_WorldIndicator* WorldIndicator;

    /** Health display that reacts to damage inflicted by world shifts. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UWidget_HealthBar* HealthBar;

    /** Blueprint-assigned class responsible for rendering world widgets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World")
    TSubclassOf<class UWidget_WorldIndicator> WorldWidgetClass;

    /** Instance of the world widget spawned at runtime. */
    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<UWidget_WorldIndicator> WorldWidgetInstance;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** Called when the loop counter changes so blueprints can update the HUD. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Loop")
    void OnLoopCountUpdated(int32 NewLoopCount);

    /** Called when an individual hint is added or changes state. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Hints")
    void OnHintUpdated(const FHintData& UpdatedHint);

    /** Called when the collection of hints changes and needs refreshing. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Hints")
    void OnHintsRefreshed();

private:
    UFUNCTION()
    void HandleLoopCountChanged(int32 NewLoopCount);

    UFUNCTION()
    void HandleHintChanged(const FHintData& UpdatedHint);

    UFUNCTION()
    void HandleHintCollectionChanged();

    /** Cached pointer to the game instance we registered with. */
    TWeakObjectPtr<UGameJamGameInstance> ObservedGameInstance;
};
