#include "Widget_Layout.h"

#include "GameJamGameInstance.h"
#include "Widget_WorldIndicator.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimeShiftEffortComponent.h"
#include "Widget_HealthBar.h"

void UWidget_Layout::NativeConstruct()
{
    Super::NativeConstruct();

    if (!WorldWidgetInstance && WorldWidgetClass)
    {
        WorldWidgetInstance = CreateWidget<UWidget_WorldIndicator>(GetWorld(), WorldWidgetClass);
        if (WorldWidgetInstance)
        {
            WorldWidgetInstance->AddToViewport();
        }
    }

    if (HealthBar)
    {
        if (APlayerController* OwningController = GetOwningPlayer())
        {
            if (APawn* Pawn = OwningController->GetPawn())
            {
                if (UTimeShiftEffortComponent* EffortComponent = Pawn->FindComponentByClass<UTimeShiftEffortComponent>())
                {
                    ObservedEffortComponent = EffortComponent;
                    if (!EffortComponent->OnEffortChanged.IsAlreadyBound(this, &UWidget_Layout::HandleEffortChanged))
                    {
                        EffortComponent->OnEffortChanged.AddDynamic(this, &UWidget_Layout::HandleEffortChanged);
                    }
                    HandleEffortChanged(EffortComponent->GetCurrentEffort());
                }
            }
        }
    }

    if (UWorld* World = GetWorld())
    {
        if (UGameJamGameInstance* GameInstance = Cast<UGameJamGameInstance>(World->GetGameInstance()))
        {
            ObservedGameInstance = GameInstance;
            GameInstance->OnLoopCountChanged.AddDynamic(this, &UWidget_Layout::HandleLoopCountChanged);
            HandleLoopCountChanged(GameInstance->GetLoopCount());

            GameInstance->OnHintChanged.AddDynamic(this, &UWidget_Layout::HandleHintChanged);
            GameInstance->OnHintCollectionChanged.AddDynamic(this, &UWidget_Layout::HandleHintCollectionChanged);
            HandleHintCollectionChanged();
        }
    }
}

void UWidget_Layout::NativeDestruct()
{
    if (UGameJamGameInstance* GameInstance = ObservedGameInstance.Get())
    {
        GameInstance->OnLoopCountChanged.RemoveDynamic(this, &UWidget_Layout::HandleLoopCountChanged);
        GameInstance->OnHintChanged.RemoveDynamic(this, &UWidget_Layout::HandleHintChanged);
        GameInstance->OnHintCollectionChanged.RemoveDynamic(this, &UWidget_Layout::HandleHintCollectionChanged);
    }

    if (UTimeShiftEffortComponent* EffortComponent = ObservedEffortComponent.Get())
    {
        EffortComponent->OnEffortChanged.RemoveDynamic(this, &UWidget_Layout::HandleEffortChanged);
    }

    ObservedEffortComponent.Reset();
    ObservedGameInstance.Reset();

    Super::NativeDestruct();
}

void UWidget_Layout::HandleLoopCountChanged(int32 NewLoopCount)
{
    OnLoopCountUpdated(NewLoopCount);
}

void UWidget_Layout::HandleHintChanged(const FHintData& UpdatedHint)
{
    OnHintUpdated(UpdatedHint);
}

void UWidget_Layout::HandleHintCollectionChanged()
{
    OnHintsRefreshed();
}

void UWidget_Layout::HandleEffortChanged(float NewEffort)
{
    if (!HealthBar)
    {
        return;
    }

    const float MaxEffort = ObservedEffortComponent.IsValid() ? ObservedEffortComponent->GetMaxEffort() : 0.0f;
    HealthBar->UpdateEffort(NewEffort, MaxEffort);
}
