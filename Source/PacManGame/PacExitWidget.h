#pragma once

// clang-format off
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "PacExitWidget.generated.h"
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
class PACMANGAME_API UPacExitWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    void OpenMenu();

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
    void HandleAcceptCloseGameClicked();

    void                  CloseMenu();
    void                  Refresh();
    APacPlayerController* GetPacController() const;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> BackDropButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> BackDropPanel;

    // Whole menu: a Border / Overlay / Canvas that holds everything below.
    // Hidden until the menu opens.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> MenuPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CancellButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> AcceptButton;

    bool bOpen = false;
};
