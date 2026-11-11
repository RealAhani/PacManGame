
// clang-format off
#include "PacTouchMenuWidget.h"
#include "Logging/PacManLog.h"
#include "PacPlayerController.h"

#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
// clang-format on

void UPacTouchMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // The widgets are bound by name; AddDynamic needs valid pointers.
    if (MenuButton != nullptr)
    {
        MenuButton->OnClicked.AddDynamic(this,
                                         &UPacTouchMenuWidget::HandleMenuClicked);
    }
    if (CloseButton != nullptr)
    {
        CloseButton->OnClicked.AddDynamic(this, &UPacTouchMenuWidget::HandleCloseClicked);
    }
    if (SwipeButton != nullptr)
    {
        SwipeButton->OnClicked.AddDynamic(this, &UPacTouchMenuWidget::HandleSwipeClicked);
    }
    if (JoystickButton != nullptr)
    {
        JoystickButton->OnClicked
            .AddDynamic(this, &UPacTouchMenuWidget::HandleJoystickClicked);
    }
    if (LeftSideButton != nullptr)
    {
        LeftSideButton->OnClicked.AddDynamic(this, &UPacTouchMenuWidget::HandleLeftClicked);
    }
    if (RightSideButton != nullptr)
    {
        RightSideButton->OnClicked
            .AddDynamic(this, &UPacTouchMenuWidget::HandleRightClicked);
    }

    // Start closed (without touching the pause state).
    bOpen = false;
    if (MenuPanel != nullptr)
    {
        MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
    }

    Refresh();
}

void UPacTouchMenuWidget::NativeDestruct()
{
    // Never leave the game paused behind.
    if (bOpen)
    {
        CloseMenu();
    }

    Super::NativeDestruct();
}

APacPlayerController* UPacTouchMenuWidget::GetPacController() const
{
    return Cast<APacPlayerController>(GetOwningPlayer());
}

void UPacTouchMenuWidget::OpenMenu()
{
    bOpen = true;

    if (MenuPanel != nullptr)
    {
        MenuPanel->SetVisibility(ESlateVisibility::Visible);
    }

    if (bPauseGameWhileOpen)
    {
        UGameplayStatics::SetGamePaused(this, true);
    }

    Refresh();
}

void UPacTouchMenuWidget::CloseMenu()
{
    bOpen = false;

    if (MenuPanel != nullptr)
    {
        MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (bPauseGameWhileOpen)
    {
        UGameplayStatics::SetGamePaused(this, false);
    }
}

// Selected choice is green, the other is grey. The side buttons only work while
// the joystick is selected.
void UPacTouchMenuWidget::Refresh()
{
    APacPlayerController const* const Controller = GetPacController();

    if (Controller == nullptr)
    {
        return;
    }

    bool const bJoystick = Controller->GetTouchScheme() == EPacTouchScheme::Joystick;
    bool const bRight = Controller->GetJoystickSide() == EPacJoystickSide::Right;

    auto const Paint = [this](UButton* Button, bool const bSelected)
    {
        if (Button != nullptr)
        {
            Button->SetBackgroundColor(bSelected ? SelectedColor : NormalColor);
        }
    };

    Paint(SwipeButton, !bJoystick);
    Paint(JoystickButton, bJoystick);
    Paint(LeftSideButton, !bRight);
    Paint(RightSideButton, bRight);

    if (LeftSideButton != nullptr)
    {
        LeftSideButton->SetIsEnabled(bJoystick);
    }
    if (RightSideButton != nullptr)
    {
        RightSideButton->SetIsEnabled(bJoystick);
    }
}

void UPacTouchMenuWidget::HandleMenuClicked()
{
    if (bOpen)
    {
        CloseMenu();
    }
    else
    {
        OpenMenu();
    }
}

void UPacTouchMenuWidget::HandleCloseClicked()
{
    CloseMenu();
}

void UPacTouchMenuWidget::HandleSwipeClicked()
{
    if (APacPlayerController* Controller = GetPacController())
    {
        Controller->SetTouchScheme(EPacTouchScheme::Swipe);
    }
    Refresh();
}

void UPacTouchMenuWidget::HandleJoystickClicked()
{
    if (APacPlayerController* Controller = GetPacController())
    {
        Controller->SetTouchScheme(EPacTouchScheme::Joystick);
    }
    Refresh();
}

void UPacTouchMenuWidget::HandleLeftClicked()
{
    if (APacPlayerController* Controller = GetPacController())
    {
        Controller->SetJoystickSide(EPacJoystickSide::Left);
    }
    Refresh();
}

void UPacTouchMenuWidget::HandleRightClicked()
{
    if (APacPlayerController* Controller = GetPacController())
    {
        Controller->SetJoystickSide(EPacJoystickSide::Right);
    }
    Refresh();
}
