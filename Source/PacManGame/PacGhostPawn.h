// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "Dev/PacDevConfig.h"

#include "PacGhostPawn.generated.h"
// clang-format on

class USphereComponent;
class UPacGhostMovementComponent;
class UPacGhostDecisionComponent;
class APacGhostAIController;

UCLASS()
class PACMANGAME_API APacGhostPawn : public APawn
{
    GENERATED_BODY()

public:

    APacGhostPawn();
    UPacGhostMovementComponent* GetGhostMovementComponent() const;
    UPacGhostDecisionComponent* GetGhostDecisionComponent() const;
#if PAC_DEV
    void KillGhost()
    {
        Destroy();
    }
#endif


private:

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USphereComponent> CollisionComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UPacGhostMovementComponent> GhostMovementComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UPacGhostDecisionComponent> GhostDecisionComponent;
};
