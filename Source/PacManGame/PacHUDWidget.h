#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PacHUDWidget.generated.h"
// clang-format on

class UTextBlock;

UCLASS()
class PACMANGAME_API UPacHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    void SetScore(int32 Score);

    void SetLives(int32 Lives);
    void ShowGameOver();
    void ShowWinning();

protected:

    virtual void NativeConstruct() override;
    virtual void NativeTick(FGeometry const & MyGeometry, float InDeltaTime) override;


private:

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ScoreText;

    // Optional: add a TextBlock with this exact name in the widget Blueprint.
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LivesText;

    // Optional: a TextBlock named GameOverText, shown when the game ends.
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> GameOverText;

    // Optional: a TextBlock named GameOverText, shown when the game ends.
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> WinningText;

    // Optional: a TextBlock named GameOverText, shown when the game ends.
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> FpsText;

    float ElapsedTime = 0.0f;
    int32 FrameCount  = 0;
};
