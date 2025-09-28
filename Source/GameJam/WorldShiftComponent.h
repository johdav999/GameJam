#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorldShiftTypes.h"
#include "WorldShiftComponent.generated.h"

class AWorldManager;

UCLASS(ClassGroup = (WorldShift), meta = (BlueprintSpawnableComponent))
class GAMEJAM_API UWorldShiftComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWorldShiftComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift")
    TArray<EWorldState> VisibleInWorlds;

    UFUNCTION(BlueprintCallable, Category = "World Shift")
    void OnWorldShift(EWorldState NewWorld);

protected:
    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void BindToWorldManager();
    void HandleWorldShift(EWorldState NewWorld);

    TWeakObjectPtr<AWorldManager> CachedWorldManager;
    FDelegateHandle WorldShiftDelegateHandle;
};
