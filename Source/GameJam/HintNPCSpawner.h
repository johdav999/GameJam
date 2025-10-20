#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HintNPCSpawner.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class AHintNPCCharacter;

UCLASS(Blueprintable, BlueprintType)
class GAMEJAM_API AHintNPCSpawner : public AActor
{
    GENERATED_BODY()

public:
    AHintNPCSpawner();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HintNPC", meta = (AllowPrivateAccess = "true"))
    UBoxComponent* TriggerVolume;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HintNPC")
    TSubclassOf<AHintNPCCharacter> NPCClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HintNPC")
    bool bSpawnOnce;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HintNPC")
    FTransform SpawnOffset;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HintNPC", meta = (AllowPrivateAccess = "true"))
    bool bHasSpawned;
};
