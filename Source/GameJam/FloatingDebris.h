#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FloatingDebris.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USceneComponent;
struct FPropertyChangedEvent;

/** Data that drives each dynamically spawned debris mesh. */
USTRUCT(BlueprintType)
struct FDebrisInstanceData
{
    GENERATED_BODY()

    FDebrisInstanceData()
        : MeshComponent(nullptr)
        , PhaseOffset(0.f)
        , RotationAxis(FVector::UpVector)
        , RotationSpeedDeg(5.f)
        , InitialRelativeLocation(FVector::ZeroVector)
    {
    }

    UPROPERTY(Transient)
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(Transient)
    float PhaseOffset;

    UPROPERTY(Transient)
    FVector RotationAxis;

    UPROPERTY(Transient)
    float RotationSpeedDeg;

    UPROPERTY(Transient)
    FVector InitialRelativeLocation;
};

/**
 * Lightweight actor that spawns floating static mesh debris with configurable oscillation and rotation.
 */
UCLASS(Blueprintable, BlueprintType)
class GAMEJAM_API AFloatingDebris : public AActor
{
    GENERATED_BODY()

public:
    AFloatingDebris();

    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Floating Motion")
    void SetFloatingEnabled(bool bInEnabled);

    UFUNCTION(BlueprintPure, Category = "Floating Motion")
    bool IsFloatingEnabled() const { return bEnableFloating; }

    UFUNCTION(BlueprintCallable, Category = "Debris")
    void RespawnDebris();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void SpawnDebrisMeshes();
    void ClearSpawnedDebris();

protected:
    /** Root scene component used for debris attachments. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    /** Box volume used to randomise initial spawn locations for debris. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* SpawnBounds;

    /** Collection of mesh variations that can be chosen for spawned debris. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debris")
    TArray<TObjectPtr<UStaticMesh>> DebrisMeshes;

    /** Number of debris mesh components to spawn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debris", meta = (ClampMin = "0"))
    int32 DebrisCount;

    /** Whether the floating motion should currently be applied. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Motion Settings")
    bool bEnableFloating;

    /** Per-axis displacement amplitude applied to floating motion (centimetres). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Motion Settings", meta = (ClampMin = "0.0"))
    FVector FloatingAmplitude;

    /** Per-axis oscillation speed expressed in cycles per second. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Motion Settings", meta = (ClampMin = "0.0"))
    FVector FloatingSpeed;

    /** Base rotational speed for debris meshes in degrees per second. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Motion Settings", meta = (ClampMin = "0.0"))
    float BaseRotationSpeedDeg;

    /** Random range added to the base rotation speed (degrees per second). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Motion Settings", meta = (ClampMin = "0.0"))
    float RotationSpeedVariance;

    /** Range used when generating random phase offsets for motion (radians). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Motion Settings", meta = (ClampMin = "0.0"))
    float RandomPhaseRange;

    /** Controls how far the random rotation axis can deviate from world up (degrees). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Motion Settings", meta = (ClampMin = "0.0", ClampMax = "180.0"))
    float RotationAxisConeHalfAngle;

private:
    TArray<FDebrisInstanceData> SpawnedDebris;

    float RunningTime;
};
