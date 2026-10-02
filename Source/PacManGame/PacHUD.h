#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PacHUD.generated.h"
// clang-format on

class UPacHUDWidget;

UCLASS()
class PACMANGAME_API APacHUD : public AHUD
{
    GENERATED_BODY()

protected:

    virtual void BeginPlay() override;

private:

    void HandleScoreChanged(int32 Score);

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UPacHUDWidget> HUDWidgetClass;

    UPROPERTY()
    TObjectPtr<UPacHUDWidget> HUDWidget;
};