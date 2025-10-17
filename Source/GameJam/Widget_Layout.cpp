#include "Widget_Layout.h"

#include "GameJamGameInstance.h"
#include "Widget_WorldIndicator.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HealthComponent.h"
#include "Widget_HealthBar.h"
#include "WorldShiftEffectsComponent.h"

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
                if (UWorldShiftEffectsComponent* Effects = Pawn->FindComponentByClass<UWorldShiftEffectsComponent>())
                {
                    if (!Effects->OnHealthDrained.IsAlreadyBound(HealthBar, &UWidget_HealthBar::UpdateHealth))
                    {
                        Effects->OnHealthDrained.AddDynamic(HealthBar, &UWidget_HealthBar::UpdateHealth);
                    }
                }

                if (UHealthComponent* HealthComponent = Pawn->FindComponentByClass<UHealthComponent>())
                {
                    if (!HealthComponent->OnHealthChanged.IsAlreadyBound(HealthBar, &UWidget_HealthBar::UpdateHealth))
                    {
                        HealthComponent->OnHealthChanged.AddDynamic(HealthBar, &UWidget_HealthBar::UpdateHealth);
                    }
                    HealthBar->UpdateHealth(HealthComponent->GetHealth(), HealthComponent->GetMaxHealth());
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
        }
    }
}

void UWidget_Layout::NativeDestruct()
{
    if (UGameJamGameInstance* GameInstance = ObservedGameInstance.Get())
    {
        GameInstance->OnLoopCountChanged.RemoveDynamic(this, &UWidget_Layout::HandleLoopCountChanged);
    }

    ObservedGameInstance.Reset();

    Super::NativeDestruct();
}

void UWidget_Layout::HandleLoopCountChanged(int32 NewLoopCount)
{
    OnLoopCountUpdated(NewLoopCount);
}
