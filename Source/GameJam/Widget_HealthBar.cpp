#include "Widget_HealthBar.h"

#include "Components/ProgressBar.h"

void UWidget_HealthBar::UpdateHealth(float NewHealth, float MaxHealth)
{
    if (!HealthProgress || MaxHealth <= 0.0f)
    {
        return;
    }

    const float Percent = FMath::Clamp(NewHealth / MaxHealth, 0.0f, 1.0f);
    HealthProgress->SetPercent(Percent);

    FLinearColor BarColor = FLinearColor::Red;

    if (Percent > 0.6f)
    {
        BarColor = FLinearColor::Green;
    }
    else if (Percent > 0.2f)
    {
        BarColor = FLinearColor::Yellow;
    }

    HealthProgress->SetFillColorAndOpacity(BarColor);
}
