#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldShiftTypes.h"
#include "TimerManager.h"
#include "WorldButton.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class USoundBase;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UWorldShiftBehaviorComponent;
struct FHitResult;
enum class EPlatformState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnButtonPressed, AWorldButton*, Button, AActor*, PressingActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnButtonReset, AWorldButton*, Button);

/**
 * Defines the visual styling for a button in a specific world state.
 */
USTRUCT(BlueprintType)
struct FWorldButtonVisualStyle
{
    GENERATED_BODY()

    FWorldButtonVisualStyle()
        : IdleColor(FLinearColor(0.35f, 0.35f, 0.35f, 1.f))
        , PressedColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.f))
        , PressedOffset(FVector(0.f, 0.f, -5.f))
    {
    }

    FWorldButtonVisualStyle(const FLinearColor& InIdleColor, const FLinearColor& InPressedColor, const FVector& InPressedOffset)
        : IdleColor(InIdleColor)
        , PressedColor(InPressedColor)
        , PressedOffset(InPressedOffset)
    {
    }

    /** Color applied while the button is idle in this world. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FLinearColor IdleColor;

    /** Color applied while the button is pressed in this world. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FLinearColor PressedColor;

    /** Relative offset applied to the button mesh when pressed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FVector PressedOffset;
};

/**
 * A world-reactive button that can trigger game logic when pressed by the player.
 */
UCLASS()
class GAMEJAM_API AWorldButton : public AActor
{
    GENERATED_BODY()

public:
    AWorldButton();

    /** Primary entry point for pressing the button from interaction code. */
    UFUNCTION(BlueprintCallable, Category = "Button")
    bool PressButton(AActor* PressingActor);

    /** Legacy helper for backward compatibility with existing Blueprints. */
    UFUNCTION(BlueprintCallable, Category = "Button")
    bool TryPressButton(AActor* PressingActor);

    /** Returns whether the button is currently pressed. */
    UFUNCTION(BlueprintPure, Category = "Button")
    bool IsPressed() const { return bIsPressed; }

    /** Returns whether the button can currently be pressed. */
    UFUNCTION(BlueprintPure, Category = "Button")
    bool CanBePressed() const;

    /** Resets the button when it supports multiple activations. */
    UFUNCTION(BlueprintCallable, Category = "Button")
    void ResetButton();

    /** Forcefully resets the button, ignoring one-time use restrictions. */
    UFUNCTION(BlueprintCallable, Category = "Button")
    void ForceResetButton();

    /** Returns the world state this button is currently visualizing. */
    UFUNCTION(BlueprintPure, Category = "Button")
    EWorldState GetCurrentVisualWorld() const { return CurrentVisualWorld; }

    /** Returns true if the supplied actor is currently overlapping the interaction volume. */
    UFUNCTION(BlueprintPure, Category = "Button")
    bool IsActorOverlappingButton(AActor* Actor) const;

    /** Native multicast delegate fired whenever the button is successfully pressed. */
    UPROPERTY(BlueprintAssignable, Category = "Button|Events")
    FOnButtonPressed OnButtonPressed;

    /** Native multicast delegate fired whenever the button resets. */
    UPROPERTY(BlueprintAssignable, Category = "Button|Events")
    FOnButtonReset OnButtonReset;

    /** Blueprint hook that fires when the button is pressed. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Button|Events")
    void ReceiveButtonPressed(AActor* PressingActor);

    /** Blueprint hook that fires when the button resets. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Button|Events")
    void ReceiveButtonReset();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Visible mesh representing the button. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> ButtonMesh;

    /** Interaction volume used to detect player overlaps. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> InteractionVolume;

    /** Component that drives world-state behavior and visibility. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UWorldShiftBehaviorComponent> WorldShiftBehavior;

    /** Optional press sound. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button|Audio")
    TObjectPtr<USoundBase> PressSound;

    /** Optional Niagara effect spawned when pressed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button|VFX")
    TObjectPtr<UNiagaraSystem> PressEffect;

    /** Whether the button should automatically activate when a pawn overlaps the volume. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
    bool bAutoPressOnOverlap;

    /** Whether the button should reset after being pressed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
    bool bAutoReset;

    /** Delay before the button resets when auto-reset is enabled. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button", meta = (EditCondition = "bAutoReset", ClampMin = "0.0"))
    float ResetDelay;

    /** When true, the button can only be pressed once. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
    bool bCanBePressedOnce;

    /** When true, the button destroys itself after a successful one-time press. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button", meta = (EditCondition = "bCanBePressedOnce"))
    bool bDestroyAfterUse;

    /**
     * Actors that should react when this button is pressed. Any actor implementing
     * UButtonInteractable will receive OnButtonActivated. Designers can list multiple
     * actors here to fan out button logic without additional scripting.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button|Interaction")
    TArray<TObjectPtr<AActor>> LinkedTargets;

    /** Default world states where the button should behave as solid when no explicit behavior is provided. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button|World")
    TArray<EWorldState> SolidWorlds;

    /** Visual styling applied when no world override is provided. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button|Visuals")
    FWorldButtonVisualStyle DefaultVisualStyle;

    /** Per-world visual overrides for idle/pressed styling. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button|Visuals")
    TMap<EWorldState, FWorldButtonVisualStyle> WorldVisualStyles;

    /** Name of the color parameter driven on the button material. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button|Visuals")
    FName ColorParameterName;

private:
    void InitializeWorldBehaviorDefaults();
    void RefreshButtonVisuals();
    void ApplyVisualStyle(const FWorldButtonVisualStyle& Style) const;
    FWorldButtonVisualStyle GetVisualStyleForWorld(EWorldState World) const;
    bool InternalPress(AActor* PressingActor);
    void HandlePressFeedback(AActor* PressingActor);
    void NotifyLinkedTargets();
    void CancelPendingReset();

    UFUNCTION()
    void HandleWorldShiftStateChanged(EPlatformState NewState, EWorldState WorldContext);

    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

    /** Returns true when the button is interactable in the current world. */
    bool IsInteractable() const;

    /** Tracks overlapping actors for blueprint queries. */
    TSet<TWeakObjectPtr<AActor>> OverlappingActors;

    /** Cached initial relative location for the button mesh. */
    FVector InitialButtonRelativeLocation;

    /** Dynamic material instance used to adjust button colors. */
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ButtonMID;

    /** Handle used for the reset timer. */
    FTimerHandle ResetTimerHandle;

    /** Cached world the button is currently visualizing. */
    EWorldState CurrentVisualWorld;

    bool bIsPressed;
    bool bHasBeenPressedOnce;
    bool bIsInteractable;
};

