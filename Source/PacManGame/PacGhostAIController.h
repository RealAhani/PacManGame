// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AIController.h"
#include "CoreMinimal.h"
#include "PacGhostAIController.generated.h"

class UStateTreeAIComponent;

UCLASS()
class PACMANGAME_API APacGhostAIController : public AAIController
{
    GENERATED_BODY()

public:

    APacGhostAIController();
    void StartStateTree();
    // Stop + start: the tree begins again at the Root (first state).
    void RestartStateTree();
    // Best effort: freezes / resumes the tree's own timers (Delay tasks).
    void SetStateTreePaused(bool bPaused);

private:

    UPROPERTY(VisibleAnywhere, Category = "AI")
    TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;
};
