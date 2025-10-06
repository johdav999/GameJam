#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widget_Layout.generated.h"

class UWidget_HealthBar;
class UWidget_WorldIndicator;
class UWorldWidget;

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
    TSubclassOf<class UWorldWidget> WorldWidgetClass;

    /** Instance of the world widget spawned at runtime. */
    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<UWorldWidget> WorldWidgetInstance;

    virtual void NativeConstruct() override;
};
