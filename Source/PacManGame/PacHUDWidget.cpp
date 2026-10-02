// clang-format off
#include "PacHUDWidget.h"

#include "Components/TextBlock.h"
// clang-format on

void UPacHUDWidget::SetScore(int32 const Score)
{
    if (ScoreText == nullptr)
    {
        return;
    }

    ScoreText->SetText(FText::FromString(FString::Printf(TEXT("SCORE: %d"), Score)));
}