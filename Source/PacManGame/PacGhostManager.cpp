// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacGhostManager.h"
#include "Logging/PacManLog.h"
#include "PacGhostDecisionComponent.h"
#include "PacGhostMovementComponent.h"
#include "PacGhostAIController.h"
#include "PacGhostPawn.h"
#include "PacMaze.h"
#include "PacGridMovementComponent.h"
#include "PacMazeDataAsset.h"

// clang-format on

APacGhostManager::APacGhostManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void APacGhostManager::Initialize(APacMaze*                  InMaze,
                                  UPacGridMovementComponent* InPlayerMovement)
{
    check(InMaze);
    check(InPlayerMovement);

    Maze           = InMaze;
    PlayerMovement = InPlayerMovement;

    SpawnGhosts();
}

void APacGhostManager::SpawnGhosts()
{
    check(Maze);

    if (GhostClass == nullptr)
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: GhostManager has no GhostClass assigned."));
        return;
    }

    UPacGridMovementComponent* PlayerMovementComponent = PlayerMovement;


    TArray<FPacGridPosition> const & GhostStarts = Maze->GetGhostStarts();

    UPacGhostMovementComponent* BlinkyMovementComponent = nullptr;

    FIntPoint const GridSize = Maze->GetGridSize();

    TArray<FPacGridPosition> const ScatterTargets =
        {FPacGridPosition(1, 1),
         FPacGridPosition(GridSize.X - 2, 1),
         FPacGridPosition(1, GridSize.Y - 2),
         FPacGridPosition(GridSize.X - 2, GridSize.Y - 2)};

    for (int32 Index = 0; Index < GhostStarts.Num(); ++Index)
    {
        FPacGridPosition const StartPosition = GhostStarts[Index];

        FVector const WorldPosition = Maze->GridToWorld(StartPosition);

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.Owner = this;

        APacGhostPawn* Ghost = GetWorld()->SpawnActor<APacGhostPawn>(GhostClass,
                                                                     WorldPosition,
                                                                     FRotator::ZeroRotator,
                                                                     SpawnParameters);

        if (Ghost == nullptr)
        {
            UE_LOG(LogPacMan,
                   Error,
                   TEXT("PAC: Failed to spawn ghost %d at (%d,%d)."),
                   Index,
                   StartPosition.X,
                   StartPosition.Y);

            continue;
        }

        UPacGhostMovementComponent* MovementComponent = Ghost->GetGhostMovementComponent();

        if (MovementComponent == nullptr)
        {
            UE_LOG(LogPacMan,
                   Error,
                   TEXT("PAC: Ghost %d has no movement component."),
                   Index);

            Ghost->Destroy();
            continue;
        }

        MovementComponent->InitializeMaze(Maze, StartPosition);

        UPacGhostDecisionComponent* DecisionComponent = Ghost->GetGhostDecisionComponent();

        if (DecisionComponent == nullptr)
        {
            UE_LOG(LogPacMan,
                   Error,
                   TEXT("PAC: Ghost %d has no decision component."),
                   Index);

            Ghost->Destroy();
            continue;
        }

        DecisionComponent->Initialize(Maze, MovementComponent);

        if (Index == 0)
        {
            BlinkyMovementComponent = MovementComponent;
        }
        DecisionComponent->SetBlinkyMovementComponent(BlinkyMovementComponent);

        EPacGhostPersonality const Personality = static_cast<EPacGhostPersonality>(
            Index % 4);

        DecisionComponent->SetPersonality(Personality);

        DecisionComponent->SetPlayerMovementComponent(PlayerMovementComponent);

        FPacGridPosition ScatterTarget = ScatterTargets[Index % ScatterTargets.Num()];

        if (!Maze->IsWalkable(ScatterTarget))
        {
            UE_LOG(LogPacMan,
                   Warning,
                   TEXT("PAC: Scatter target (%d,%d) is blocked; "
                        "using ghost start position instead."),
                   ScatterTarget.X,
                   ScatterTarget.Y);

            ScatterTarget = StartPosition;
        }

        DecisionComponent->SetScatterTarget(ScatterTarget);

        // Start the StateTree only after ghost initialization.
        APacGhostAIController* AIController = Cast<APacGhostAIController>(
            Ghost->GetController());

        if (AIController == nullptr)
        {
            UE_LOG(LogPacMan,
                   Error,
                   TEXT("PAC: Ghost %d has no PacGhostAIController. StateTree "
                        "was not started."),
                   Index);
        }
        else
        {
            AIController->StartStateTree();
        }

        Ghosts.Add(Ghost);

        UE_LOG(LogPacMan,
               Log,
               TEXT("PAC: Ghost %d initialized at (%d,%d)."),
               Index,
               StartPosition.X,
               StartPosition.Y);
    }

    UE_LOG(LogPacMan, Log, TEXT("PAC: Spawned %d ghosts."), Ghosts.Num());
}
