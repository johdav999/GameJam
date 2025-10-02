#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldShiftTypes.h"
#include "ShiftPlatform.generated.h"

class UStaticMeshComponent;
class UWorldShiftBehaviorComponent;

UENUM(BlueprintType)
enum class EPlatformState : uint8
{
    Solid UMETA(DisplayName = "Solid"),
    Ghost UMETA(DisplayName = "Ghost"),
    Hidden UMETA(DisplayName = "Hidden"),
    TimedSolid UMETA(DisplayName = "Timed Solid")
};

UENUM(BlueprintType)
enum class EPlatformPrefabType : uint8
{
    LightBridge UMETA(DisplayName = "Light Bridge"),
    LightToShadow UMETA(DisplayName = "Light to Shadow"),
    ShadowBridge UMETA(DisplayName = "Shadow Bridge"),
    ShadowIllusion UMETA(DisplayName = "Shadow Illusion"),
    ChaosFlicker UMETA(DisplayName = "Chaos Flicker"),
    ChaosTrap UMETA(DisplayName = "Chaos Trap"),
    ChaosBridge UMETA(DisplayName = "Chaos Bridge"),
    ShiftChain UMETA(DisplayName = "Shift Chain"),
    HiddenSurprise UMETA(DisplayName = "Hidden Surprise"),
    DeceptionPlatform UMETA(DisplayName = "Deception Platform")
};

UCLASS()
class GAMEJAM_API AShiftPlatform : public AActor
{
    GENERATED_BODY()

public:
    AShiftPlatform();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> PlatformMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Shift")
    TObjectPtr<UWorldShiftBehaviorComponent> WorldShiftBehavior;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Shift|Prefab")
    EPlatformPrefabType PrefabType;
};
