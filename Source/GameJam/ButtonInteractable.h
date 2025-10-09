#pragma once

#include "UObject/Interface.h"

#include "ButtonInteractable.generated.h"

class AWorldButton;

/**
 * Interface implemented by actors that can respond to AWorldButton activations.
 * Use this to keep button interactions fully data-driven and avoid hard references.
 */
UINTERFACE(BlueprintType)
class GAMEJAM_API UButtonInteractable : public UInterface
{
    GENERATED_BODY()
};

class GAMEJAM_API IButtonInteractable
{
    GENERATED_BODY()

public:
    /**
     * Called whenever a linked world button is activated.
     * Implementations can drive any custom behavior (open doors, toggle FX, etc.).
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Button")
    void OnButtonActivated(AWorldButton* Button);
};

