// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacGhostManager.h"
#include "PacGhostDecisionComponent.h"
#include "PacGhostMovementComponent.h"
#include "PacGhostAIController.h"
#include "PacGhostPawn.h"
#include "PacMaze.h"
#include "PacGridMovementComponent.h"
#include "PacMazeDataAsset.h"

#include "kismet/GameplayStatics.h"
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

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

    if (PlayerPawn == nullptr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("Pac: GhostManager could not find the player pawn."));
        return;
    }

    UPacGridMovementComponent* PlayerMovementComponent = PlayerPawn->FindComponentByClass<
        UPacGridMovementComponent>();
    if (PlayerMovementComponent == nullptr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("Pac: Player Pawn '%s' has no PacGridMovementComponent."),
               *GetNameSafe(PlayerPawn));
        return;
    }

    UE_LOG(LogTemp,
           Warning,
           TEXT("Pac: GhostManager found player movement component '%s'."),
           *GetNameSafe(PlayerMovementComponent));


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
            UE_LOG(LogTemp,
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
            UE_LOG(LogTemp,
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

        UE_LOG(LogTemp,
               Warning,
               TEXT("PAC: Ghost %d initialized at (%d,%d)."),
               Index,
               StartPosition.X,
               StartPosition.Y);
    }

    UE_LOG(LogTemp, Warning, TEXT("PAC: Spawned %d ghosts."), Ghosts.Num());
}
