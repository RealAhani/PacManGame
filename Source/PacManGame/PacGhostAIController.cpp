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