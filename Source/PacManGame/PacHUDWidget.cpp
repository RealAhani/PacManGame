// clang-format off
#include "PacHUDWidget.h"

#include "Components/TextBlock.h"
// clang-format on
void UPacHUDWidget::NativeTick(FGeometry const & MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (InDeltaTime <= 0.0f)
    {
        return;
    }

    ElapsedTime += InDeltaTime;
    ++FrameCount;

    if (ElapsedTime >= 0.5f)
    {
        float const FPS = static_cast<float>(FrameCount) / ElapsedTime;

        if (FpsText)
        {
            FpsText->SetText(
                FText::FromString(FString::Printf(TEXT("FPS: %.0f"), FPS)));
        }

        ElapsedTime = 0.0f;
        FrameCount  = 0;
    }
}

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

void UPacHUDWidget::ShowWinning()
{
    if (WinningText != nullptr)
    {
        WinningText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void UPacHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (GameOverText != nullptr)
    {
        GameOverText->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (WinningText != nullptr)
    {
        WinningText->SetVisibility(ESlateVisibility::Collapsed);
    }
}
