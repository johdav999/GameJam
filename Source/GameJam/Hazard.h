#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShiftPlatform.h"
#include "WorldShiftTypes.h"
#include "Hazard.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UWorldShiftBehaviorComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class UAudioComponent;
class UHealthComponent;

UCLASS()
class GAMEJAM_API AHazard : public AActor
{
    GENERATED_BODY()

public:
    AHazard();

protected:
    virtual void BeginPlay() override;

    /** Collision box for detecting overlaps */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> CollisionBox;

    /** Visual mesh */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> HazardMesh;

    /** Controls visibility/activation in different worlds */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
    TObjectPtr<UWorldShiftBehaviorComponent> WorldShiftBehavior;

    /** Particle and audio feedback */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
    TObjectPtr<UNiagaraSystem> ActiveEffect;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TObjectPtr<USoundBase> ActiveSound;

    /** Damage applied to player */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float DamageAmount = 20.f;

    /** Whether damage applies continuously (every second) or once on touch */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    bool bContinuousDamage = false;

private:
    /** Called when player overlaps hazard */
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    /** Called when player leaves the hazard */
    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    UFUNCTION()
    void HandleWorldStateChanged(EPlatformState NewState, EWorldState WorldContext);

    void DealDamageToActor(AActor* Target);
    void ApplyContinuousDamage();
    void UpdateActivation(bool bNewActive);
    bool IsSolidState(EPlatformState State) const;

    /** Whether the hazard is currently active (solid) */
    bool bIsActive = true;

    /** Overlapping actor cache for continuous damage */
    TSet<TWeakObjectPtr<AActor>> OverlappingActors;

    /** Timer for continuous damage ticks */
    FTimerHandle DamageTickHandle;

    UPROPERTY(Transient)
    TObjectPtr<UNiagaraComponent> ActiveEffectComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> ActiveAudioComponent;
};
