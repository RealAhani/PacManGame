// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "PacGhostDecisionComponent.h"
#include "StateTreeTaskBase.h"
#include "PacGhostSetBehaviorModeTask.generated.h"
// clang-format on

class APacGhostPawn;

USTRUCT()
struct FPacGhostSetBehaviorModeTaskInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Context")
    TObjectPtr<APacGhostPawn> GhostPawn = nullptr;

    UPROPERTY(EditAnywhere, Category = "Parameter")
    EPacGhostBehaviorMode BehaviorMode = EPacGhostBehaviorMode::Scatter;
};

USTRUCT(meta = (DisplayName = "Set Ghost Behavior Mode", Category = "PacMan|Ghost"))
struct PACMANGAME_API FPacGhostSetBehaviorModeTask : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

    using FInstanceDataType = FPacGhostSetBehaviorModeTaskInstanceData;

    virtual UStruct const* GetInstanceDataType() const override
    {
        return FInstanceDataType::StaticStruct();
    }

    virtual EStateTreeRunStatus EnterState(
        FStateTreeExecutionContext &       Context,
        FStateTreeTransitionResult const & Transition) const override;
};
