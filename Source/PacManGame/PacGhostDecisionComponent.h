#pragma once

// clang-format off
#include "PacGridTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "PacGhostDecisionComponent.generated.h"
// clang-format on

class APacMaze;
class UPacGhostMovementComponent;
class UPacGridMovementComponent;

UENUM(BlueprintType)
enum class EPacGhostBehaviorMode : uint8
{
    None,
    Scatter,
    Chase,
    Frightened,
    Eaten
};

UENUM(BlueprintType)
enum class EPacGhostPersonality : uint8
{
    Blinky,
    Pinky,
    Inky,
    Clyde
};

// Where the ghost is relative to the ghost house. Independent of the behaviour
// mode: a ghost waiting in the house is still "Scatter" underneath.
UENUM(BlueprintType)
enum class EPacGhostHouseState : uint8
{
    Outside,  // on the streets; the house door is closed to it
    InHouse,  // waiting inside, bobbing up and down
    Leaving   // walking to the house exit
};

// Fired when a new Scatter round is requested (the start of a wave).
DECLARE_MULTICAST_DELEGATE(FOnScatterRoundStarted);


// Lets a Blueprint react (change material, play a sound, ...) when the mode changes.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPacGhostModeChanged,
                                            EPacGhostBehaviorMode,
                                            NewMode);


UCLASS(ClassGroup = (PacMan), meta = (BlueprintSpawnableComponent))
class PACMANGAME_API UPacGhostDecisionComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UPacGhostDecisionComponent();

    void Initialize(APacMaze*                   InMaze,
                    UPacGhostMovementComponent* InMovementComponent);
    void SetScatterTarget(FPacGridPosition TargetPosition);
    void SetPlayerMovementComponent(UPacGridMovementComponent* InPlayerMovementComponent);
    void SetPersonality(EPacGhostPersonality NewPersonality);
    // Where an Eaten ghost returns to (its start tile).
    void SetHomePosition(FPacGridPosition InHomePosition);
    void SetBlinkyMovementComponent(UPacGhostMovementComponent* InBlinkyMovementComponent);
    EPacGhostBehaviorMode GetBehaviorMode() const
    {
        return BehaviorMode;
    }

    // Only a ghost that is hunting (Scatter / Chase) can hurt Pac-Man.
    // Frightened and Eaten ghosts are harmless.
    bool IsDangerous() const
    {
        return BehaviorMode == EPacGhostBehaviorMode::Scatter ||
               BehaviorMode == EPacGhostBehaviorMode::Chase;
    }

    // Call after the movement component was put back on its start tile.
    void Respawn();

    // -------------------- ghost house --------------------
    // Call before Initialize() and again on every reset. bStartInHouse: the
    // ghost starts inside the house and waits to be released. ExitPosition: the
    // street tile outside the door.
    void ConfigureHouse(bool bStartInHouse, FPacGridPosition ExitPosition);

    bool IsInHouse() const
    {
        return HouseState == EPacGhostHouseState::InHouse;
    }

    EPacGhostHouseState GetHouseState() const
    {
        return HouseState;
    }

    // Walk to the exit and go out. The door closes behind the ghost.
    void StartLeavingHouse();

    // Forget which Scatter/Chase request came last, so the next Scatter request
    // counts as a new wave (see OnScatterRoundStarted).
    void ResetWaveTracking();
    // The wave this life starts in (1 = first wave). After a death the manager
    // sets it to the wave the player died in, so the StateTree can skip the
    // earlier Scatter/Chase rounds (see FPacStartWaveCondition).
    void SetStartWave(int32 InStartWave)
    {
        StartWave = FMath::Max(1, InStartWave);
    }

    int32 GetStartWave() const
    {
        return StartWave;
    }


    FOnScatterRoundStarted OnScatterRoundStarted;


    UPROPERTY(BlueprintAssignable, Category = "Ghost AI")
    FPacGhostModeChanged OnModeChanged;

    // Scatter / Chase are "base" modes: they are remembered, and applied
    // unless the ghost is Frightened or Eaten right now (the StateTree keeps
    // sending them on its own schedule). Frightened / Eaten apply at once.
    UFUNCTION(BlueprintCallable, Category = "Ghost AI")
    void SetBehaviorMode(EPacGhostBehaviorMode NewMode);

    // Ends Frightened / Eaten and goes back to the remembered base mode.
    void ReleaseOverride();

    EPacGhostBehaviorMode GetBaseMode() const
    {
        return BaseMode;
    }


private:

    void HandleGridPositionChanged([[maybe_unused]] FPacGridPosition const & Position);
    void ChooseDirection();
    void ApplyMode(EPacGhostBehaviorMode NewMode);
    bool ChooseDirectionTowardTarget(FPacGridPosition const & TargetPosition);
    // Picks a random open direction, never reversing unless at a dead end.
    // Used for wandering modes and when the ghost is standing on its target.
    bool ChooseRandomDirection();
    // Waiting in the house: wander up and down between house tiles.
    bool ChooseDirectionInHouse();
    // True if this ghost may walk on house tiles right now.
    bool CanEnterHouse() const;
    void UpdateHousePermission();
    bool IsDirectionWalkable(FPacGridPosition const & Position,
                             EPacDirection            Direction) const;
    void HandlePlayerGridPositionChanged(
        [[maybe_unused]] FPacGridPosition const & Position);

    UPROPERTY(VisibleInstanceOnly, Category = "Ghost AI")
    EPacGhostPersonality Personality = EPacGhostPersonality::Blinky;

    UPROPERTY()
    TObjectPtr<APacMaze> Maze;

    UPROPERTY()
    TObjectPtr<UPacGhostMovementComponent> MovementComponent;

    UPROPERTY()
    TObjectPtr<UPacGhostMovementComponent> BlinkyMovementComponent;

    UPROPERTY()
    TObjectPtr<UPacGridMovementComponent> PlayerMovementComponent;

    UPROPERTY(VisibleInstanceOnly, Category = "Ghost AI")
    EPacGhostBehaviorMode BehaviorMode = EPacGhostBehaviorMode::None;

    // The Scatter / Chase mode the schedule (StateTree) last asked for.
    UPROPERTY(VisibleInstanceOnly, Category = "Ghost AI")
    EPacGhostBehaviorMode BaseMode = EPacGhostBehaviorMode::Scatter;

    UPROPERTY(VisibleInstanceOnly, Category = "Ghost AI")
    FPacGridPosition ScatterTargetPosition;

    UPROPERTY(VisibleInstanceOnly, Category = "Ghost AI")
    bool bHasScatterTarget = false;

    UPROPERTY(EditDefaultsOnly, Category = "Ghost AI", meta = (ClampMin = "0.1"))
    float FrightenedSpeedMultiplier = 0.6f;

    UPROPERTY(EditDefaultsOnly, Category = "Ghost AI", meta = (ClampMin = "0.1"))
    float EatenSpeedMultiplier = 2.0f;

    UPROPERTY(VisibleInstanceOnly, Category = "Ghost AI")
    EPacGhostHouseState HouseState = EPacGhostHouseState::Outside;

    // The last Scatter/Chase mode asked for (by the StateTree or the schedule).
    EPacGhostBehaviorMode LastRequestedHuntMode = EPacGhostBehaviorMode::None;
    FPacGridPosition      HomePosition;
    FPacGridPosition      HouseExitPosition;
    bool                  bHasHomePosition            = false;
    bool                  bAllowReverseOnNextDecision = false;
    int32                 StartWave                   = 1;
};
