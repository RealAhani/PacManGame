#pragma once

// clang-format off
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "PacTouchMenuWidget.generated.h"
// clang-format on

class UButton;
class UWidget;
class UVerticalBox;
class APacPlayerController;

// A small settings menu: a button on screen opens a panel where the player picks
// Swipe or Joystick (and which side the joystick is on).
// Build the layout in a Widget Blueprint whose parent class is this one. The
// widgets below must exist there with exactly these names.
UCLASS()
class PACMANGAME_API UPacTouchMenuWidget : public UUserWidget
{
    GENERATED_BODY()

protected:

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:

    // -------------------- functions --------------------
    UFUNCTION()
    void HandleMenuClicked();
    UFUNCTION()
    void HandleCloseClicked();
    UFUNCTION()
    void HandleSwipeClicked();
    UFUNCTION()
    void HandleJoystickClicked();
    UFUNCTION()
    void HandleLeftClicked();
    UFUNCTION()
    void HandleRightClicked();

    void                  OpenMenu();
    void                  CloseMenu();
    void                  Refresh();
    APacPlayerController* GetPacController() const;

    // -------------------- widgets (names must match in the Blueprint)
    // -------------------- The small button that is always on screen.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> BackDropButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> BackDropPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MenuButton;

    // Whole menu: a Border / Overlay / Canvas that holds everything below.
    // Hidden until the menu opens.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> MenuPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> SwipeButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> JoystickButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CloseButton;

    // Optional: only used when the joystick is selected.
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> LeftSideButton;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> RightSideButton;

    // -------------------- properties --------------------
    // Pause the game while the menu is open.
    UPROPERTY(EditDefaultsOnly, Category = "Menu")
    bool bPauseGameWhileOpen = true;

    UPROPERTY(EditDefaultsOnly, Category = "Menu")
    FLinearColor SelectedColor = FLinearColor(0.1f, 0.6f, 0.2f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Menu")
    FLinearColor NormalColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);

    bool bOpen = false;
};
