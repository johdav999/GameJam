#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorldShiftTypes.h"
#include "WorldShiftComponent.generated.h"

class AWorldManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldShiftComponentSignature, EWorldState, NewWorld);

UCLASS(ClassGroup = (WorldShift), meta = (BlueprintSpawnableComponent))
class GAMEJAM_API UWorldShiftComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWorldShiftComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift")
    TArray<EWorldState> VisibleInWorlds;

    /** Broadcast when the world shifts. */
    UPROPERTY(BlueprintAssignable, Category = "World Shift")
    FOnWorldShiftComponentSignature OnWorldShifted;

    UFUNCTION(BlueprintCallable, Category = "World Shift")
    virtual void OnWorldShift(EWorldState NewWorld);

protected:
    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void BindToWorldManager();
    UFUNCTION()
    void HandleWorldShift(EWorldState NewWorld);

    TWeakObjectPtr<AWorldManager> CachedWorldManager;
};
