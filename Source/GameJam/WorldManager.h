#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldManager.generated.h"

UENUM(BlueprintType)
enum class EWorldState : uint8
{
    Light UMETA(DisplayName = "Light"),
    Shadow UMETA(DisplayName = "Shadow"),
    Dream UMETA(DisplayName = "Dream")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldShiftedSignature, EWorldState, NewWorld);

class UWorldShiftComponent;

UCLASS(Blueprintable)
class GAMEJAM_API AWorldManager : public AActor
{
    GENERATED_BODY()

public:
    AWorldManager();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "World")
    EWorldState GetCurrentWorld() const { return CurrentWorld; }

    UFUNCTION(BlueprintCallable, Category = "World")
    void SetWorld(EWorldState NewWorld);

    UFUNCTION(BlueprintCallable, Category = "World")
    void CycleWorld(int32 Direction);

    UPROPERTY(BlueprintAssignable, Category = "World")
    FOnWorldShiftedSignature OnWorldShifted;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World")
    EWorldState CurrentWorld;

    UFUNCTION(BlueprintCallable, Category = "World")
    static AWorldManager* GetWorldManager(const UObject* WorldContextObject);

protected:
    void BroadcastWorldShift(EWorldState NewWorld);
};
