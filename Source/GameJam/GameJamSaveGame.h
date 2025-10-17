#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "HintTypes.h"
#include "GameJamSaveGame.generated.h"

/**
 * Simple save game object that persists the loop counter between sessions.
 */
UCLASS()
class GAMEJAM_API UGameJamSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UGameJamSaveGame();

    /** Number of times the player has experienced a world reset. */
    UPROPERTY(BlueprintReadWrite, Category = "Loop")
    int32 LoopCount;

    /** Persistent hints that should survive between play sessions. */
    UPROPERTY(BlueprintReadWrite, Category = "Hints")
    TArray<FHintData> PersistentHints;
};
