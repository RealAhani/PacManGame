// clang-format off
#include "PacHUDWidget.h"

#include "Components/TextBlock.h"
// clang-format on

void UPacHUDWidget::SetScore(const int32 Score) {
  if (ScoreText == nullptr) {
    return;
  }

  ScoreText->SetText(
      FText::FromString(FString::Printf(TEXT("SCORE: %d"), Score)));
}