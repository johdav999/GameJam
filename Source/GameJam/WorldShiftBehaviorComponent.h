#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorldShiftTypes.h"
#include "WorldState.h"
#include "WorldShiftBehaviorComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class AWorldManager;
enum class EPlatformPrefabType : uint8;
enum class EPlatformState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldShiftMaterialStateChanged, EWorldMaterialState, NewState);

/**
 * Component that applies world shift behavior to an owning actor.
 * The component is responsible for reacting to world changes, swapping
 * materials, toggling collision, and driving ghost hint visualization.
 */
UCLASS(ClassGroup = (WorldShift), BlueprintType, meta = (BlueprintSpawnableComponent))
class GAMEJAM_API UWorldShiftBehaviorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWorldShiftBehaviorComponent();

    /** Root mesh that will receive the visibility, collision, and material changes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Setup")
    TObjectPtr<UStaticMeshComponent> TargetMesh;

    /** Optional ghost hint mesh that is toggled based on future solid states. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Ghost Hint")
    TObjectPtr<UStaticMeshComponent> GhostHintMesh;

    /** Per-world behavior mapping. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift")
    TMap<EWorldState, EPlatformState> WorldBehaviors;

    /** Material applied when the mesh is solid. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Materials")
    TObjectPtr<UMaterialInterface> SolidMaterial;

    /** Material applied during the global pre-warning window. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Materials")
    TObjectPtr<UMaterialInterface> PreWarningMaterial;

    /** Ghost materials keyed by the world they correspond to. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Materials")
    TMap<EWorldState, TObjectPtr<UMaterialInterface>> GhostMaterials;

    /** Hint materials keyed by the world where the platform becomes solid. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Materials")
    TMap<EWorldState, TObjectPtr<UMaterialInterface>> GhostHintMaterials;

    /** Accessor to the current world tracked by the component. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "World Shift")
    EWorldState CurrentWorld;

    /** Accessor to the current platform state tracked by the component. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "World Shift")
    EPlatformState CurrentState;

    /** Accessor to the current material state tracked by the component. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "World Shift")
    EWorldMaterialState CurrentMaterialState;

    /** Assigns the mesh that should receive visibility/material updates. */
    void SetTargetMesh(UStaticMeshComponent* InMesh);

    /** Assigns the optional ghost hint mesh managed by the component. */
    void SetGhostHintMesh(UStaticMeshComponent* InMesh);

    /** Clears and sets up the behavior map from a prefab definition. */
    void EnsureWorldBehaviorsFromPrefab(EPlatformPrefabType PrefabType);

    /** Clears the behavior map. */
    void ResetWorldBehaviors();

    /** Forces the ghost hint mesh to refresh using the provided preview world. */
    void RefreshGhostHintPreview(EWorldState PreviewWorld);

    /** Blueprint hook fired when the platform state changes. */
    UFUNCTION(BlueprintImplementableEvent, Category = "World Shift|Events")
    void OnShiftStateChanged(EPlatformState NewState, EWorldState WorldContext);

    /** Native event fired when the material/collision state changes. */
    UPROPERTY(BlueprintAssignable, Category = "World Shift|Events")
    FOnWorldShiftMaterialStateChanged OnMaterialStateChanged;

    /** Returns the current material state the component represents. */
    UFUNCTION(BlueprintPure, Category = "World Shift")
    EWorldMaterialState GetCurrentMaterialState() const { return CurrentMaterialState; }

    /** Blueprint hook fired when the pre-warning window begins. */
    UFUNCTION(BlueprintImplementableEvent, Category = "World Shift|Events")
    void OnPreWarningStarted(bool bWillBeSolid);

    /** Blueprint hook fired when the ghost hint mesh gets updated. */
    UFUNCTION(BlueprintImplementableEvent, Category = "World Shift|Events")
    void OnGhostHintUpdated();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnRegister() override;

private:
    void InitializeFromOwner();
    void BindToWorldManager();

    UFUNCTION()
    void HandleWorldShift(EWorldState NewWorld);

    UFUNCTION()
    void HandleGlobalTimedSolidPhaseChanged(bool bNowSolid);

    UFUNCTION()
    void HandleGlobalTimedSolidPreWarning(bool bWillBeSolid);

    void ApplyPlatformState(EPlatformState NewState, EWorldState WorldContext);
    void ApplySolidState();
    void ApplyGhostState(EWorldState WorldContext);
    void ApplyHiddenState();
    void ApplyPreWarningState();
    void ApplyMaterial(UMaterialInterface* Material) const;

    void BroadcastMaterialState(EWorldMaterialState NewState);
    EWorldMaterialState ResolveMaterialState(EPlatformState PlatformState) const;

    void UpdateGhostHint(EWorldState WorldContext);

    EPlatformState GetBehaviorForWorld(EWorldState WorldContext) const;

    void UnbindFromWorldManager();

    TWeakObjectPtr<AWorldManager> CachedWorldManager;
};

