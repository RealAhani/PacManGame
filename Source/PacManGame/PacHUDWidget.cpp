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

void UPacHUDWidget::SetLives(int32 const Lives)
{
    if (LivesText == nullptr)
    {
        return;
    }

    LivesText->SetText(FText::FromString(FString::Printf(TEXT("LIVES: %d"), Lives)));
}

void UPacHUDWidget::ShowGameOver()
{
    if (GameOverText != nullptr)
    {
        GameOverText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void UPacHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (GameOverText != nullptr)
    {
        GameOverText->SetVisibility(ESlateVisibility::Collapsed);
    }
}
