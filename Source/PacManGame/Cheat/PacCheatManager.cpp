#include "PacCheatManager.h"
#include "PacPlayerPawn.h"
#include "PacGameMode.h"
#include "PacGhostManager.h"


void UPacCheatManager::PacGodMode(bool bEnabled)
{
    APlayerController* PlayerController = GetPlayerController();
    if (PlayerController == nullptr)
    {
        return;
    }
    APacPlayerPawn* Player = Cast<APacPlayerPawn>(PlayerController->GetPawn());
    if (Player == nullptr)
    {
        return;
    }
    Player->SetGodMode(bEnabled);
}

void UPacCheatManager::PacGhostsKillAll()
{
    APacGameMode* GameMode = GetWorld()->GetAuthGameMode<APacGameMode>();
    if (GameMode == nullptr)
    {
        return;
    }
    APacGhostManager* GhostManager = GameMode->GetGhostManager();
    if (GhostManager == nullptr)
    {
        return;
    }
    GhostManager->KillAllGhost();
}

void UPacCheatManager::PacLose()
{
    APacGameMode* GameMode = GetWorld()->GetAuthGameMode<APacGameMode>();
    if (GameMode == nullptr)
    {
        return;
    }
    GameMode->PacLose();
}

void UPacCheatManager::PacWin()
{
    APacGameMode* GameMode = GetWorld()->GetAuthGameMode<APacGameMode>();
    if (GameMode == nullptr)
    {
        return;
    }
    GameMode->PacWin();
}
