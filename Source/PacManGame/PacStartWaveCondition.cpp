// clang-format off
#include "PacStartWaveCondition.h"
#include "Logging/PacManLog.h"
#include "PacGhostDecisionComponent.h"
#include "PacGhostPawn.h"
#include "StateTreeExecutionContext.h"
// clang-format on

bool FPacStartWaveCondition::TestCondition(FStateTreeExecutionContext & Context) const
{
    FPacStartWaveConditionInstanceData const & InstanceData = Context.GetInstanceData(
        *this);

    if (!IsValid(InstanceData.GhostPawn))
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: Wave condition has no GhostPawn."));
        return false;
    }

    UPacGhostDecisionComponent const*
        DecisionComponent = InstanceData.GhostPawn->GetGhostDecisionComponent();

    if (!IsValid(DecisionComponent))
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: Ghost has no DecisionComponent."));
        return false;
    }

    return InstanceData.Wave >= DecisionComponent->GetStartWave();
}
