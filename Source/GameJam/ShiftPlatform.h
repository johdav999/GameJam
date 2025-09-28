#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShiftPlatform.generated.h"

class UStaticMeshComponent;
class UWorldShiftComponent;

UCLASS()
class GAMEJAM_API AShiftPlatform : public AActor
{
    GENERATED_BODY()

public:
    AShiftPlatform();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> PlatformMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Shift")
    TObjectPtr<UWorldShiftComponent> WorldShiftComponent;
};
