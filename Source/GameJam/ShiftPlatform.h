#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldShiftTypes.h"
#include "ShiftPlatform.generated.h"

class UStaticMeshComponent;
class UWorldShiftComponent;
class UMaterialInterface;
class AWorldManager;
struct FTimerHandle;

UENUM(BlueprintType)
enum class EPlatformState : uint8
{
    Solid UMETA(DisplayName = "Solid"),
    Ghost UMETA(DisplayName = "Ghost"),
    Hidden UMETA(DisplayName = "Hidden"),
    TimedSolid UMETA(DisplayName = "Timed Solid")
};

UCLASS()
class GAMEJAM_API AShiftPlatform : public AActor
{
    GENERATED_BODY()

public:
    AShiftPlatform();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> PlatformMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Shift")
    TObjectPtr<UWorldShiftComponent> WorldShiftComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift")
    TMap<EWorldState, EPlatformState> WorldBehaviors;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Materials")
    TObjectPtr<UMaterialInterface> SolidMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Materials")
    TMap<EWorldState, TObjectPtr<UMaterialInterface>> GhostMaterials;

private:
    UFUNCTION()
    void HandleWorldShift(EWorldState NewWorld);

    void ApplyPlatformState(EPlatformState NewState, EWorldState WorldContext);
    void ApplySolidState();
    void ApplyGhostState(EWorldState WorldContext);
    void ApplyHiddenState();

    void StartTimedSolidCycle();
    void StopTimedSolidCycle();
    void HandleTimedSolidToggle();

    EPlatformState GetBehaviorForWorld(EWorldState WorldContext) const;
    void ApplyMaterial(UMaterialInterface* Material);

    TWeakObjectPtr<AWorldManager> CachedWorldManager;

    FTimerHandle TimedSolidTimerHandle;

    EWorldState CurrentWorld;
    bool bTimedSolidCurrentlySolid;
};
