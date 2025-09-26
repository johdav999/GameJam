#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "PaintZone.generated.h"

class UBoxComponent;
class UDecalComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EForceType : uint8
{
    Push    UMETA(DisplayName = "Push"),
    Pull    UMETA(DisplayName = "Pull"),
    Gravity UMETA(DisplayName = "Gravity")
};

UCLASS()
class APaintZone : public AActor
{
    GENERATED_BODY()

public:
    APaintZone();

    /** Configures the paint zone after it has been spawned. */
    void InitializePaintZone(EForceType InForceType, const FVector& InSurfaceNormal, bool bInPermanent);

    /** Initializes the zone based on a trace hit. */
    void InitializeFromHit(const FHitResult& Hit, EForceType ForceType);

    /** Returns true if this zone is permanent. */
    bool IsPermanent() const { return bPermanent; }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    /** Updates the visual state (color + orientation) of the paint. */
    void UpdateVisuals();

    /** Applies the configured force to every overlapping actor. */
    void ApplyForces(float DeltaSeconds);

protected:
    /** Root component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    /** Visual representation of the paint splat. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UDecalComponent* DecalComponent;

    /** Collision volume used to determine the affected area. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* OverlapComponent;

    /** Optional dynamic material for tinting and fading. */
    UPROPERTY(Transient)
    UMaterialInstanceDynamic* DecalMID;

    /** Force type applied by this zone. */
    UPROPERTY(BlueprintReadOnly, Category = "Paint Zone")
    EForceType ForceType = EForceType::Push;

    /** Unit direction that represents the painted surface normal. */
    UPROPERTY(BlueprintReadOnly, Category = "Paint Zone")
    FVector SurfaceNormal = FVector::UpVector;

    /** True if this paint zone should never disappear. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paint Zone")
    bool bPermanent = false;

    /** Duration before the zone begins to fade out. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paint Zone", meta = (ClampMin = "0.0", Units = "s"))
    float LifeTime = 5.0f;

    /** How long the zone takes to fade away after it expires. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paint Zone", meta = (ClampMin = "0.0", Units = "s"))
    float FadeOutDuration = 1.0f;

    /** Magnitude of the push/pull forces applied, treated as acceleration (cm/s^2). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paint Zone", meta = (ClampMin = "0.0"))
    float ForceStrength = 2500.0f;

    /** Magnitude of the gravity-altering force, treated as acceleration (cm/s^2). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paint Zone", meta = (ClampMin = "0.0"))
    float GravityStrength = 980.0f;

    /** Color tint used for push zones. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paint Zone|Visuals")
    FLinearColor PushColor = FLinearColor::Red;

    /** Color tint used for pull zones. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paint Zone|Visuals")
    FLinearColor PullColor = FLinearColor::Blue;

    /** Color tint used for gravity zones. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paint Zone|Visuals")
    FLinearColor GravityColor = FLinearColor::Green;

    /** Material parameter used to tint the decal. */
    UPROPERTY(EditDefaultsOnly, Category = "Paint Zone|Visuals")
    FName ColorParameterName = TEXT("TintColor");

private:
    /** Accumulated lifetime for fade/force management. */
    float ElapsedTime = 0.0f;
};

