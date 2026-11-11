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
    virtual void DrawHUD() override;

private:

    void HandleScoreChanged(int32 Score);
    void HandleLivesChanged(int32 Lives);
    void HandleGameOver();
    void HandleLevelCompelet();

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UPacHUDWidget> HUDWidgetClass;

    UPROPERTY()
    TObjectPtr<UPacHUDWidget> HUDWidget;

    UPROPERTY(EditDefaultsOnly, Category = "UI|Joystick")
    TObjectPtr<UTexture2D> JoystickBaseTexture;

    UPROPERTY(EditDefaultsOnly, Category = "UI|Joystick")
    TObjectPtr<UTexture2D> JoystickThumbTexture;
};
