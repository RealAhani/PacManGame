#pragma once

// clang-format off
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "PacHUDWidget.generated.h"
// clang-format on

class UTextBlock;

UCLASS()
class PACMANGAME_API UPacHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    void SetScore(int32 Score);

private:

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ScoreText;
};