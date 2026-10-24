
// clang-format off
#include "PacGhostSetBehaviorModeTask.h"

#include "PacGhostDecisionComponent.h"
#include "PacGhostPawn.h"
#include "StateTreeExecutionContext.h"
// clang-format on

EStateTreeRunStatus FPacGhostSetBehaviorModeTask::EnterState(
    FStateTreeExecutionContext &       Context,
    FStateTreeTransitionResult const & Transition) const
{
    FPacGhostSetBehaviorModeTaskInstanceData const & InstanceData = Context.GetInstanceData(
        *this);

    if (!IsValid(InstanceData.GhostPawn))
    {
        UE_LOG(LogTemp, Error, TEXT("PAC: SetBehaviorMode task has no GhostPawn."));

        return EStateTreeRunStatus::Failed;
    }

    UPacGhostDecisionComponent*
        DecisionComponent = InstanceData.GhostPawn->GetGhostDecisionComponent();

    if (!IsValid(DecisionComponent))
    {
        UE_LOG(LogTemp, Error, TEXT("PAC: Ghost has no DecisionComponent."));

        return EStateTreeRunStatus::Failed;
    }

    DecisionComponent->SetBehaviorMode(InstanceData.BehaviorMode);

    return EStateTreeRunStatus::Running;
}
