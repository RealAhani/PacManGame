// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacGameMode.h"
#include "PacMaze.h"
#include "PacPelletManager.h"
#include "PacPlayerState.h"
#include "PacGhostManager.h"
#include "PacPlayerPawn.h"
#include "PacGridMovementComponent.h"
#include "Logging/PacManLog.h"

#include "Kismet/GameplayStatics.h"
// clang-format on

APacGameMode::APacGameMode()
{
    PlayerStateClass = APacPlayerState::StaticClass();
}

void APacGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!InitializeLevel())
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: Level initialization failed, the game will not run. "
                    "See the errors above."));
    }
}

// BeginPlay order between different actors (GameMode, PlayerController, Pawn,
// Maze...) is not guaranteed. So only this function wires them together, in an
// order that makes each step's inputs ready:
//   1. find everything, fail loudly if something is missing
//   2. camera -> maze
//   3. player pawn learns the maze (so its grid position is valid)
//   4. pellets (need the maze) and the score / level-complete bindings
//   5. ghosts last, because they read the player's grid position
bool APacGameMode::InitializeLevel()
{
    UWorld* World = GetWorld();

    // ---- 1. find everything ----
    Maze = Cast<APacMaze>(
        UGameplayStatics::GetActorOfClass(World, APacMaze::StaticClass()));
    if (Maze == nullptr)
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: No APacMaze placed in the level."));
        return false;
    }
    if (!Maze->IsRuntimeInitialized())
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: Maze '%s' is not initialized. Is MazeData assigned "
                    "and valid?"),
               *Maze->GetName());
        return false;
    }

    PelletManager = Cast<APacPelletManager>(
        UGameplayStatics::GetActorOfClass(World, APacPelletManager::StaticClass()));
    if (PelletManager == nullptr)
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: No APacPelletManager placed in the level."));
        return false;
    }

    GhostManager = Cast<APacGhostManager>(
        UGameplayStatics::GetActorOfClass(World, APacGhostManager::StaticClass()));
    if (GhostManager == nullptr)
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: No APacGhostManager placed in the level."));
        return false;
    }

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (PlayerController == nullptr)
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: There is no player controller."));
        return false;
    }

    APacPlayerPawn* PlayerPawn = Cast<APacPlayerPawn>(PlayerController->GetPawn());
    if (PlayerPawn == nullptr)
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: Player controller '%s' has no APacPlayerPawn. "
                    "Check "
                    "DefaultPawnClass in the GameMode."),
               *PlayerController->GetName());
        return false;
    }

    UPacGridMovementComponent* PlayerMovement = PlayerPawn->GetGridMovementComponent();
    if (PlayerMovement == nullptr)
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: Player pawn has no grid movement component."));
        return false;
    }

    // ---- 2. camera ----
    PlayerController->SetViewTarget(Maze);

    // ---- 3. player ----
    PlayerPawn->InitializeMaze(Maze);

    // ---- 4. pellets, score, level complete ----
    PelletManager->Initialize(Maze);
    PelletManager->BindToPlayerMovement(PlayerMovement);
    PelletManager->OnPelletConsumed.AddUObject(this, &APacGameMode::HandlePelletConsumed);
    PelletManager->OnAllPelletsConsumed
        .AddUObject(this, &APacGameMode::HandleAllPelletsConsumed);

    // ---- 5. ghosts ----
    GhostManager->Initialize(Maze, PlayerMovement);

    UE_LOG(LogPacMan,
           Log,
           TEXT("PAC: Level initialized with maze '%s'."),
           *Maze->GetName());
    return true;
}

void APacGameMode::HandlePelletConsumed(int32 const ScoreValue)
{
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

    APacPlayerState*
        PacPlayerState = PlayerController != nullptr
                             ? PlayerController->GetPlayerState<APacPlayerState>()
                             : nullptr;

    if (PacPlayerState == nullptr)
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: Cannot add score. No APacPlayerState."));
        return;
    }

    PacPlayerState->AddScore(ScoreValue);
}

APacMaze* APacGameMode::GetMaze() const
{
    return Maze;
}

APacPelletManager* APacGameMode::GetPelletManager() const
{
    return PelletManager;
}

void APacGameMode::HandleAllPelletsConsumed()
{
    UE_LOG(LogPacMan, Log, TEXT("PAC: All pellets consumed. Level complete."));
}

APacGhostManager* APacGameMode::GetGhostManager() const
{
    return GhostManager;
}
