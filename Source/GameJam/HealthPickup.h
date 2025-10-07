#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HealthPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UWorldShiftBehaviorComponent;
class USoundBase;
class UHealthComponent;

UCLASS()
class GAMEJAM_API AHealthPickup : public AActor
{
    GENERATED_BODY()

public:
    AHealthPickup();

protected:
    virtual void BeginPlay() override;

    /** Collision for overlap detection */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> CollisionSphere;

    /** Visual mesh */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> PickupMesh;

    /** World behavior component that defines Solid/Ghost/etc. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
    TObjectPtr<UWorldShiftBehaviorComponent> WorldShiftBehavior;

    /** Pickup sound */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TObjectPtr<USoundBase> PickupSound;

    /** Amount of health to add */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float HealthAmount = 10.f;

private:
    /** Handle player overlap */
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    /** Checks if the pickup is currently collectible */
    bool CanBeCollected() const;
};
