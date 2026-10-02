// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacGhostManager.h"
#include "PacGhostDecisionComponent.h"
#include "PacGhostMovementComponent.h"

#include "PacGhostPawn.h"
#include "PacMaze.h"
#include "PacMazeDataAsset.h"
// clang-format on

APacGhostManager::APacGhostManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void APacGhostManager::Initialize(APacMaze* InMaze)
{
    check(InMaze);

    Maze = InMaze;

    SpawnGhosts();
}

void APacGhostManager::SpawnGhosts()
{
    check(Maze);

    if (GhostClass == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("PAC: GhostManager has no GhostClass assigned."));
        return;
    }

    TArray<FPacGridPosition> const & GhostStarts = Maze->GetGhostStarts();

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
            UE_LOG(LogTemp,
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
            UE_LOG(LogTemp,
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
            UE_LOG(LogTemp,
                   Error,
                   TEXT("PAC: Ghost %d has no decision component."),
                   Index);

            Ghost->Destroy();
            continue;
        }

        DecisionComponent->Initialize(Maze, MovementComponent);

        Ghosts.Add(Ghost);

        UE_LOG(LogTemp,
               Warning,
               TEXT("PAC: Ghost %d initialized at (%d,%d)."),
               Index,
               StartPosition.X,
               StartPosition.Y);
    }

    UE_LOG(LogTemp, Warning, TEXT("PAC: Spawned %d ghosts."), Ghosts.Num());
}