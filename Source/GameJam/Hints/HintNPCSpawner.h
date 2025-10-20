#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AITypes.h"
#include "HintNPCSpawner.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class AAIController;
class AHintNPCCharacter;
struct FPathFollowingResult;

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

    void HandleMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

    void CleanupActiveNPC();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hint NPC", meta = (AllowPrivateAccess = "true"))
    UBoxComponent* TriggerVolume;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hint NPC", meta = (AllowPrivateAccess = "true"))
    UBoxComponent* TargetLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint NPC")
    TSubclassOf<AHintNPCCharacter> NPCClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint NPC")
    bool bSpawnOnce;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint NPC")
    FTransform SpawnOffset;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hint NPC", meta = (AllowPrivateAccess = "true"))
    bool bHasSpawned;

    UPROPERTY()
    TWeakObjectPtr<AHintNPCCharacter> ActiveNPC;

    TWeakObjectPtr<AAIController> ActiveNPCController;

    FAIRequestID ActiveMoveRequestID;
};
