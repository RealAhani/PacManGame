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
    PrimaryActorTick.bCanEverTick = true;
    PlayerStateClass              = APacPlayerState::StaticClass();
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

    PlayerPawn = Cast<APacPlayerPawn>(PlayerController->GetPawn());
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
    PelletManager->OnPelletConsumed.AddUObject(this, &APacGameMode::AddScoreToPlayer);
    PelletManager->OnPowerPelletConsumed
        .AddUObject(this, &APacGameMode::HandlePowerPelletConsumed);
    PelletManager->OnAllPelletsConsumed
        .AddUObject(this, &APacGameMode::HandleAllPelletsConsumed);

    // ---- 5. ghosts ----
    GhostManager->Initialize(Maze, PlayerMovement);
    GhostManager->OnGhostEaten.AddUObject(this, &APacGameMode::AddScoreToPlayer);

    // ---- 6. rules ----
    if (APacPlayerState* PacPlayerState = PlayerController
                                              ->GetPlayerState<APacPlayerState>())
    {
        PacPlayerState->SetLives(StartingLives);
    }
    else
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: No APacPlayerState, lives will not work."));
        return false;
    }


    UE_LOG(LogPacMan,
           Log,
           TEXT("PAC: Level initialized with maze '%s'."),
           *Maze->GetName());

    BeginReady();
    return true;
}

// READY: everything frozen for a moment, then play starts. Used at level start
// and after every death.
void APacGameMode::BeginReady()
{
    SetGameState(EPacGameState::Ready);
    SetActorsMovementEnabled(false);

    GetWorldTimerManager().SetTimer(ReadyTimerHandle,
                                    this,
                                    &APacGameMode::StartPlaying,
                                    FMath::Max(ReadySeconds, KINDA_SMALL_NUMBER),
                                    false);
}

void APacGameMode::StartPlaying()
{
    SetActorsMovementEnabled(true);
    SetGameState(EPacGameState::Playing);
}

void APacGameMode::HandlePowerPelletConsumed()
{
    if (PacManGameState == EPacGameState::Playing && GhostManager != nullptr)
    {
        GhostManager->StartFrightened();
    }
}

void APacGameMode::AddScoreToPlayer(int32 const ScoreValue)
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
    if (PacManGameState != EPacGameState::Playing)
    {
        return;
    }

    SetGameState(EPacGameState::LevelComplete);
    SetActorsMovementEnabled(false);
    UE_LOG(LogPacMan, Log, TEXT("PAC: All pellets consumed. Level complete."));
    OnLevelCompelet.Broadcast();
}

APacGhostManager* APacGameMode::GetGhostManager() const
{
    return GhostManager;
}

void APacGameMode::Tick(float const DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (PacManGameState == EPacGameState::Playing)
    {
        CheckGhostCollision();
    }
}

// Collision is a distance test on the maze plane, not a physics overlap. Both
// sides move continuously between tile centres, so this also catches the
// "swap" case (Pac-Man and a ghost passing through each other on one edge)
// that a same-tile check would miss.
void APacGameMode::CheckGhostCollision()
{
    if (PlayerPawn == nullptr || GhostManager == nullptr || Maze == nullptr)
    {
        return;
    }

    float const HitDistance = Maze->GetTileSize() * HitDistanceInTiles;

    // Also eats frightened ghosts; returns true only if Pac-Man was caught.
    if (GhostManager->UpdateContacts(PlayerPawn->GetActorLocation(), HitDistance))
    {
        HandlePlayerCaught();
    }
}

void APacGameMode::HandlePlayerCaught()
{
    SetGameState(EPacGameState::Dying);
    SetActorsMovementEnabled(false);

    APacPlayerState* PacPlayerState = PlayerPawn->GetPlayerState<APacPlayerState>();

    if (PacPlayerState == nullptr)
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: Player pawn has no APacPlayerState."));
    }
    else
    {
        PacPlayerState->LoseLife();
    }

    GetWorldTimerManager().SetTimer(DeathTimerHandle,
                                    this,
                                    &APacGameMode::FinishDeath,
                                    FMath::Max(DeathPauseSeconds, KINDA_SMALL_NUMBER),
                                    false);
}

void APacGameMode::FinishDeath()
{
    APacPlayerState* PacPlayerState = PlayerPawn != nullptr
                                          ? PlayerPawn->GetPlayerState<APacPlayerState>()
                                          : nullptr;

    if (PacPlayerState == nullptr || PacPlayerState->GetLives() <= 0)
    {
        SetGameState(EPacGameState::GameOver);

        UE_LOG(LogPacMan, Log, TEXT("PAC: Game over."));

        OnGameOver.Broadcast();
        GetWorldTimerManager()
            .SetTimer(DeathTimerHandle,
                      this,
                      &APacGameMode::RestartLevel,
                      FMath::Max(DeathPauseSeconds, KINDA_SMALL_NUMBER),
                      false);
        BeginReady();
        return;
    }

    // Back to the start tiles, then everyone moves again.
    PlayerPawn->InitializeMaze(Maze);
    GhostManager->ResetGhosts();

    BeginReady();
}

void APacGameMode::SetActorsMovementEnabled(bool const bEnabled)
{
    if (PlayerPawn != nullptr)
    {
        PlayerPawn->SetMovementEnabled(bEnabled);
    }

    if (GhostManager != nullptr)
    {
        GhostManager->SetMovementEnabled(bEnabled);
    }
}

void APacGameMode::SetGameState(EPacGameState const NewState)
{
    if (PacManGameState == NewState)
    {
        return;
    }

    UE_LOG(LogPacMan,
           Log,
           TEXT("PAC: Game state %s -> %s"),
           *UEnum::GetValueAsString(PacManGameState),
           *UEnum::GetValueAsString(NewState));

    PacManGameState = NewState;
}

void APacGameMode::RestartLevel() const
{
    FString const LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
    UGameplayStatics::OpenLevel(this, FName(*LevelName));
}
