// clang-format off
#include "PacGhostAIController.h"

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
        UE_LOG(LogTemp,
               Error,
               TEXT("PAC: Cannot start StateTree. StateTreeAIComponent is "
                    "null."));
        return;
    }

    UE_LOG(LogTemp,
           Error,
           TEXT("PAC: Controller=%s | Controlled Pawn=%s"),
           *GetName(),
           *GetNameSafe(GetPawn()));

    StateTreeAIComponent->StartLogic();

    UE_LOG(LogTemp, Log, TEXT("PAC: Ghost StateTree started."));
}
