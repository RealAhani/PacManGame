#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "PacGridTypes.h"
#include "PacGhostDecisionComponent.h"
#include "PacGhostPawn.h"

#include "Dev/PacDevConfig.h"

#include "PacGhostManager.generated.h"
// clang-format on

class APacMaze;
class APacGhostPawn;
class UPacGridMovementComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGhostEaten, int32);

// One step of the Scatter/Chase schedule.
USTRUCT()
struct FPacModePhase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Phase")
    EPacGhostBehaviorMode Mode = EPacGhostBehaviorMode::Scatter;

    // How long this phase lasts. 0 = forever (use for the last phase).
    UPROPERTY(EditAnywhere, Category = "Phase", meta = (ClampMin = "0.0"))
    float Seconds = 7.0f;
};


UCLASS()
class PACMANGAME_API APacGhostManager : public AActor
{
    GENERATED_BODY()

public:

    APacGhostManager();
    virtual void Tick(float DeltaSeconds) override;

    void Initialize(APacMaze* InMaze, UPacGridMovementComponent* InPlayerMovement);
    FOnGhostEaten OnGhostEaten;

    // Resolves ghost contact with Pac-Man at Location (maze plane, world
    // units, within HitDistance):
    //   - a Frightened ghost is eaten (becomes Eaten, OnGhostEaten fires)
    //   - a Scatter / Chase ghost catches Pac-Man -> returns true
    bool UpdateContacts(FVector const & PlayerLocation, float HitDistance);

    // Starts / stops the simulation: ghost movement AND the mode timers.
    void SetMovementEnabled(bool bEnabled);
    // Called when a power pellet is eaten: every ghost becomes Frightened for
    // FrightenedSeconds, and the Scatter/Chase schedule pauses meanwhile.
    void StartFrightened();

    // Puts every ghost back on its start tile.
    void ResetGhosts();

#if PAC_DEV
    void KillAllGhost()
    {
        for (APacGhostPawn* Ghost : Ghosts)
        {
            if (IsValid(Ghost))
            {
                Ghost->KillGhost();
            }
        }
    }

#endif

private:

    void SpawnGhosts();

    UPROPERTY(EditDefaultsOnly, Category = "Ghosts")
    TSubclassOf<APacGhostPawn> GhostClass;

    UPROPERTY()
    TObjectPtr<APacMaze> Maze;
    UPROPERTY()
    TObjectPtr<UPacGridMovementComponent> PlayerMovement;

    UPROPERTY()
    TArray<TObjectPtr<APacGhostPawn>> Ghosts;

    // Same order as Ghosts.
    TArray<FPacGridPosition> GhostStartPositions;

    // -------------------- mode schedule --------------------
    EPacGhostBehaviorMode GetBaseMode() const;
    void                  ApplyBaseMode();
    void                  AdvanceSchedule(float DeltaSeconds);
    void                  RestartSchedule(int32 StartWave = 1);
    int32                 FindSchedulePhaseForWave(int32 Wave) const;
    void                  EndFrightened();
    void                  ReviveArrivedGhosts();
    void                  SetStateTreesPaused(bool bPaused);
    void                  RestartStateTrees();
    void                  StopStateTrees();
    void                  DrawDebugModes() const;
    //
    // -------------------- ghost house / waves --------------------
    // Called by the first ghost's decision component at the start of every
    // Scatter round: wave N lets ghost N out of the house.
    void HandleScatterRoundStarted();
    void ReleaseGhost(int32 GhostIndex);
    void ArmWaveClock();
    void TickReleases(float DeltaSeconds);


    // Scatter / Chase timeline. Paused while ghosts are frightened.
    UPROPERTY(EditDefaultsOnly, Category = "Ghosts|Schedule")
    TArray<FPacModePhase> ModeSchedule;

    UPROPERTY(EditDefaultsOnly, Category = "Ghosts|Schedule", meta = (ClampMin = "0.1"))
    float FrightenedSeconds = 6.0f;

    // Score for the first ghost eaten per power pellet; doubles each time.
    UPROPERTY(EditDefaultsOnly, Category = "Ghosts|Schedule", meta = (ClampMin = "0"))
    int32 GhostScoreBase = 200;


    // true : ST_Ghost (the StateTree) owns the Scatter/Chase timeline, and
    //        ModeSchedule is ignored. Frightened / Eaten are layered on top.
    // false: ModeSchedule below drives it from C++ and the tree is not started.
    UPROPERTY(EditDefaultsOnly, Category = "Ghosts|Schedule")
    bool bUseStateTree = true;

    // After a death the next ghosts leave the house one after another, this
    // many seconds apart, instead of all in the same frame.
    UPROPERTY(EditDefaultsOnly, Category = "Ghosts|House", meta = (ClampMin = "0"))
    float ReleaseStaggerSeconds = 0.8f;

    // Draws each ghost's current mode above it (compiled out in Shipping).
    UPROPERTY(EditAnywhere, Category = "Ghosts|Debug")
    bool bShowDebugModes = true;

    int32 PhaseIndex         = 0;
    float PhaseTimeLeft      = 0.0f;
    float FrightenedTimeLeft = 0.0f;
    int32 GhostsEatenInChain = 0;
    bool  bRunning           = false;

    // The trees are (re)started when play begins, not at spawn / reset: during
    // the Ready pause the round timers must not be running yet.
    bool bStateTreesNeedStart = false;

    // The current wave (one Scatter round). It is NOT reset when the player
    // dies: the new life starts at the beginning of the same wave and that
    // many ghosts leave the house again. Level reload resets it.
    // Rounds are ignored until play has begun (armed), so resets and spawning
    // do not release ghosts.
    int32 WaveCount       = 0;
    bool  bWaveClockArmed = false;

    // true after a death: the first Scatter round of the new life is the
    // start of wave WaveCount again, not a new wave.
    bool bResumingWave = false;

    // Ghost indices still waiting to be let out (see ReleaseStaggerSeconds).
    TArray<int32> PendingReleases;
    float         ReleaseCooldown = 0.0f;
};
