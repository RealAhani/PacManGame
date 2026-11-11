#include "PacExitWidget.h"
#include "Logging/PacManLog.h"
#include "PacPlayerController.h"

#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"


void UPacExitWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // The widgets are bound by name; AddDynamic needs valid pointers.
    if (BackDropButton != nullptr)
    {
        BackDropButton->OnClicked.AddDynamic(this, &UPacExitWidget::HandleCloseClicked);
    }
    if (CancellButton != nullptr)
    {
        CancellButton->OnClicked.AddDynamic(this, &UPacExitWidget::HandleCloseClicked);
    }
    if (AcceptButton != nullptr)
    {
        AcceptButton->OnClicked
            .AddDynamic(this, &UPacExitWidget::HandleAcceptCloseGameClicked);
    }

    // Start closed (without touching the pause state).
    bOpen = false;
    if (MenuPanel != nullptr)
    {
        MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (BackDropPanel != nullptr)
    {
        BackDropPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UPacExitWidget::NativeDestruct()
{
    // Never leave the game paused behind.
    if (bOpen)
    {
        CloseMenu();
    }

    Super::NativeDestruct();
}

void UPacExitWidget::HandleMenuClicked()
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

void UPacExitWidget::HandleCloseClicked()
{
    CloseMenu();
}

void UPacExitWidget::HandleAcceptCloseGameClicked()
{
    APacPlayerController* PC = GetPacController();
    if (PC != nullptr)
    {
        PC->ClosePacGame();
    }
}

APacPlayerController* UPacExitWidget::GetPacController() const
{
    return Cast<APacPlayerController>(GetOwningPlayer());
}

// it will called from player controller when user press back button
void UPacExitWidget::OpenMenu()
{
    bOpen = true;

    if (MenuPanel != nullptr)
    {
        MenuPanel->SetVisibility(ESlateVisibility::Visible);
    }
    if (BackDropPanel != nullptr)
    {
        BackDropPanel->SetVisibility(ESlateVisibility::Visible);
    }
}

void UPacExitWidget::CloseMenu()
{
    bOpen = false;

    if (MenuPanel != nullptr)
    {
        MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (BackDropPanel != nullptr)
    {
        BackDropPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}
