#include "PacCheatManager.h"
#include "../Dev/PacDevConfig.h"

#if PAC_DEV
#include "../PacPlayerPawn.h"
#include "../PacGameMode.h"
#include "../PacGhostManager.h"
#endif


void UPacCheatManager::PacGodMode(bool bEnabled)
{
#if PAC_DEV
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
#endif
}

void UPacCheatManager::PacGhostsKillAll()
{
#if PAC_DEV
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
#endif
}

void UPacCheatManager::PacLose()
{
#if PAC_DEV
    APacGameMode* GameMode = GetWorld()->GetAuthGameMode<APacGameMode>();
    if (GameMode == nullptr)
    {
        return;
    }
    GameMode->PacLose();
#endif
}

void UPacCheatManager::PacWin()
{
#if PAC_DEV
    APacGameMode* GameMode = GetWorld()->GetAuthGameMode<APacGameMode>();
    if (GameMode == nullptr)
    {
        return;
    }
    GameMode->PacWin();
#endif
}
