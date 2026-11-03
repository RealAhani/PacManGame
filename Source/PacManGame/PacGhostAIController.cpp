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

    // Make sure a pause from before the restart does not carry over.
    StateTreeAIComponent->ResumeLogic(TEXT("Pac restart"));

    UE_LOG(LogPacMan,
           Log,
           TEXT("PAC: Ghost StateTree started for %s."),
           *GetNameSafe(GetPawn()));
}

void APacGhostAIController::StopStateTree()
{
    if (StateTreeAIComponent != nullptr)
    {
        StateTreeAIComponent->StopLogic(TEXT("Pac stop"));
    }
}

void APacGhostAIController::SetStateTreePaused(bool const bPaused)
{
    if (StateTreeAIComponent == nullptr || !StateTreeAIComponent->IsRunning())
    {
        return;
    }

    // No IsPaused() check on purpose: calling Resume on a running tree is
    // harmless, while trusting IsPaused() could leave a tree stuck paused.
    if (bPaused)
    {
        StateTreeAIComponent->PauseLogic(TEXT("Pac frightened"));
    }
    else
    {
        StateTreeAIComponent->ResumeLogic(TEXT("Pac resumed"));
    }
}
