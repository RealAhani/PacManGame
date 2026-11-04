// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PacGameMode.generated.h"
// clang-format on

class APacMaze;
class APacPlayerPawn;
class APacPelletManager;
class APacPlayerState;
class APacGhostManager;

UENUM()
enum class EPacGameState : uint8
{
    Initializing,  // before InitializeLevel() succeeded
    Ready,         // "READY!" pause, everything frozen, then play starts
    Playing,
    Dying,         // short freeze after Pac-Man was caught
    GameOver,
    LevelComplete
};

DECLARE_MULTICAST_DELEGATE(FOnPacGameOver);
DECLARE_MULTICAST_DELEGATE(FOnPacLevelCompelet);

UCLASS()
class PACMANGAME_API APacGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:

    APacGameMode();
    virtual void       BeginPlay() override;
    virtual void       Tick(float DeltaSeconds) override;
    APacMaze*          GetMaze() const;
    APacPelletManager* GetPelletManager() const;
    APacGhostManager*  GetGhostManager() const;
    EPacGameState      GetGameState() const
    {
        return PacManGameState;
    }
    void                RestartLevel() const;
    FOnPacGameOver      OnGameOver;
    FOnPacLevelCompelet OnLevelCompelet;

private:

    void HandleAllPelletsConsumed();
    // Wires the whole level together in ONE fixed order (see the .cpp).
    // Nothing else may depend on BeginPlay order between actors.
    bool InitializeLevel();
    void AddScoreToPlayer(int32 Amount);
    void HandlePowerPelletConsumed();
    // -------------------- collision / lives --------------------
    void BeginReady();
    void StartPlaying();
    void CheckGhostCollision();
    void HandlePlayerCaught();
    void FinishDeath();
    void SetActorsMovementEnabled(bool bEnabled);
    void SetGameState(EPacGameState NewState);

    // Number of lives at the start of a game.
    UPROPERTY(EditDefaultsOnly, Category = "Rules", meta = (ClampMin = "1"))
    int32 StartingLives = 3;

    // The pause before play starts, at level start and after each death.
    UPROPERTY(EditDefaultsOnly, Category = "Rules", meta = (ClampMin = "0.0"))
    float ReadySeconds = 2.0f;


    // How long everything stays frozen after Pac-Man is caught.
    UPROPERTY(EditDefaultsOnly, Category = "Rules", meta = (ClampMin = "0.0"))
    float DeathPauseSeconds = 1.5f;

    // Pac-Man and a ghost touch when closer than this many tiles.
    UPROPERTY(EditDefaultsOnly, Category = "Rules", meta = (ClampMin = "0.1"))
    float HitDistanceInTiles = 0.5f;

    UPROPERTY(VisibleInstanceOnly, Category = "Rules")
    EPacGameState PacManGameState = EPacGameState::Initializing;

    UPROPERTY()
    TObjectPtr<APacPlayerPawn> PlayerPawn;

    FTimerHandle DeathTimerHandle;
    FTimerHandle ReadyTimerHandle;


    UPROPERTY()
    TObjectPtr<APacMaze> Maze;

    UPROPERTY()
    TObjectPtr<APacPelletManager> PelletManager;

    UPROPERTY()
    TObjectPtr<APacGhostManager> GhostManager;
};
