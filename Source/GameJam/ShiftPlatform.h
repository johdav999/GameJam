#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldShiftTypes.h"
#include "ShiftPlatform.generated.h"

class UStaticMeshComponent;
class UWorldShiftComponent;
class UMaterialInterface;
class AWorldManager;

UENUM(BlueprintType)
enum class EPlatformState : uint8
{
    Solid UMETA(DisplayName = "Solid"),
    Ghost UMETA(DisplayName = "Ghost"),
    Hidden UMETA(DisplayName = "Hidden"),
    TimedSolid UMETA(DisplayName = "Timed Solid")
};

UENUM(BlueprintType)
enum class EPlatformPrefabType : uint8
{
    LightBridge UMETA(DisplayName = "Light Bridge"),
    LightToShadow UMETA(DisplayName = "Light to Shadow"),
    ShadowBridge UMETA(DisplayName = "Shadow Bridge"),
    ShadowIllusion UMETA(DisplayName = "Shadow Illusion"),
    ChaosFlicker UMETA(DisplayName = "Chaos Flicker"),
    ChaosTrap UMETA(DisplayName = "Chaos Trap"),
    ChaosBridge UMETA(DisplayName = "Chaos Bridge"),
    ShiftChain UMETA(DisplayName = "Shift Chain"),
    HiddenSurprise UMETA(DisplayName = "Hidden Surprise"),
    DeceptionPlatform UMETA(DisplayName = "Deception Platform")
};

UCLASS()
class GAMEJAM_API AShiftPlatform : public AActor
{
    GENERATED_BODY()

public:
    AShiftPlatform();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> PlatformMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Shift")
    TObjectPtr<UWorldShiftComponent> WorldShiftComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Shift|Prefab")
    EPlatformPrefabType PrefabType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift")
    TMap<EWorldState, EPlatformState> WorldBehaviors;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Materials")
    TObjectPtr<UMaterialInterface> SolidMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Materials")
    TObjectPtr<UMaterialInterface> PreWarningMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Materials")
    TMap<EWorldState, TObjectPtr<UMaterialInterface>> GhostMaterials;

    UFUNCTION(BlueprintImplementableEvent, Category = "World Shift|Events")
    void OnPreWarningStarted(bool bWillBeSolid);

private:
    UFUNCTION()
    void HandleWorldShift(EWorldState NewWorld);

    void ApplyPlatformState(EPlatformState NewState, EWorldState WorldContext);
    void ApplySolidState();
    void ApplyGhostState(EWorldState WorldContext);
    void ApplyHiddenState();
    void ApplyPreWarningState();

    UFUNCTION()
    void OnGlobalTimedSolidPhaseChanged(bool bNowSolid);

    UFUNCTION()
    void OnGlobalTimedSolidPreWarning(bool bWillBeSolid);

    EPlatformState GetBehaviorForWorld(EWorldState WorldContext) const;
    void ApplyMaterial(UMaterialInterface* Material);
    void InitializeWorldBehaviorsFromPrefab();

    TWeakObjectPtr<AWorldManager> CachedWorldManager;

    EWorldState CurrentWorld;
};
