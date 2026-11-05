#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "PacStartWaveCondition.generated.h"
// clang-format on

class APacGhostPawn;

USTRUCT()
struct FPacStartWaveConditionInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Context")
    TObjectPtr<APacGhostPawn> GhostPawn = nullptr;

    // The wave this state belongs to (Scatter1/Chase1 = 1, Scatter2/Chase2 = 2...).
    UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "1"))
    int32 Wave = 1;
};

// Enter Condition for the Scatter/Chase states of ST_Ghost.
// True when this state's wave has not been skipped: Wave >= the wave this life
// starts in. After a death in wave 3, the states of waves 1 and 2 fail the
// condition, so the tree enters Scatter3 and the wave does not restart.
USTRUCT(meta = (DisplayName = "Ghost Wave Not Skipped", Category = "PacMan|Ghost"))
struct PACMANGAME_API FPacStartWaveCondition : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()

    using FInstanceDataType = FPacStartWaveConditionInstanceData;

    virtual UStruct const* GetInstanceDataType() const override
    {
        return FInstanceDataType::StaticStruct();
    }

    virtual bool TestCondition(FStateTreeExecutionContext & Context) const override;
};
