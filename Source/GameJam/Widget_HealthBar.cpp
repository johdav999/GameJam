#include "Widget_HealthBar.h"

#include "Components/ProgressBar.h"

void UWidget_HealthBar::UpdateEffort(float NewEffort, float MaxEffort)
{
    if (!HealthProgress || MaxEffort <= 0.0f)
    {
        return;
    }

    const float Percent = FMath::Clamp(NewEffort / MaxEffort, 0.0f, 1.0f);
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
