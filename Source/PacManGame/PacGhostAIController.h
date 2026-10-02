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

private:

    UPROPERTY(VisibleAnywhere, Category = "AI")
    TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;
};