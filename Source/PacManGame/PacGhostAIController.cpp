// clang-format off
#include "PacGhostAIController.h"
#include "Logging/PacManLog.h"

#include "Components/StateTreeAIComponent.h"
// clang-format on

APacGhostAIController::APacGhostAIController()
{
    StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(
        TEXT("StateTreeAIComponent"));

    StateTreeAIComponent->SetStartLogicAutomatically(false);
}

void APacGhostAIController::StartStateTree()
{
    if (StateTreeAIComponent == nullptr)
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: Cannot start StateTree. StateTreeAIComponent is "
                    "null."));
        return;
    }

    UE_LOG(LogPacMan,
           Verbose,
           TEXT("PAC: Controller=%s | Controlled Pawn=%s"),
           *GetName(),
           *GetNameSafe(GetPawn()));

    StateTreeAIComponent->StartLogic();

    UE_LOG(LogPacMan, Log, TEXT("PAC: Ghost StateTree started."));
}

void APacGhostAIController::RestartStateTree()
{
    if (StateTreeAIComponent == nullptr)
    {
        return;
    }

    StateTreeAIComponent->StopLogic(TEXT("Pac restart"));
    StateTreeAIComponent->StartLogic();
}

void APacGhostAIController::SetStateTreePaused(bool const bPaused)
{
    if (StateTreeAIComponent == nullptr || !StateTreeAIComponent->IsRunning())
    {
        return;
    }

    if (bPaused && !StateTreeAIComponent->IsPaused())
    {
        StateTreeAIComponent->PauseLogic(TEXT("Pac frozen"));
    }
    else if (!bPaused && StateTreeAIComponent->IsPaused())
    {
        StateTreeAIComponent->ResumeLogic(TEXT("Pac resumed"));
    }
}
