#pragma once

// clang-format off
#include "PacGridTypes.h"

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "PacGhostMovementComponent.generated.h"
// clang-format on

class APacMaze;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGhostGridPositionChanged,
                                    FPacGridPosition const &);

UCLASS(ClassGroup = (PacMan), meta = (BlueprintSpawnableComponent))
class PACMANGAME_API UPacGhostMovementComponent : public UPawnMovementComponent
{
    GENERATED_BODY()

public:

    UPacGhostMovementComponent();
    void InitializeMaze(APacMaze* InMaze, FPacGridPosition const & StartPosition);
    void SetDirection(EPacDirection Direction);
    // Freezes / unfreezes movement (death pause, level complete, game over).
    void SetMovementEnabled(bool bEnabled);
    // 1 = normal. Frightened ghosts are slower, eaten ghosts faster.
    void             SetSpeedMultiplier(float Multiplier);
    FPacGridPosition GetCurrentGridPosition() const;
    EPacDirection    GetTravelDirection() const;
    bool             IsMoving() const
    {
        return CurrentGridPosition != TargetGridPosition;
    }


    FOnGhostGridPositionChanged OnGridPositionChanged;

protected:

    virtual void TickComponent(float      DeltaTime,
                               ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:

    bool TryStartMovement();
    // Moves up to MaxDistance toward the target tile. Returns the distance
    // that was NOT used (greater than zero only if the target was reached),
    // so the caller can carry it into the next tile.
    float MoveTowardTarget(float MaxDistance);

    // True while the current move is a tunnel step and Pac-Man / the ghost has
    // not yet left the map on its way to the other side.
    bool bCrossingTunnel = false;


    UPROPERTY(EditInstanceOnly, Category = "Grid Movement")
    TObjectPtr<APacMaze> Maze;

    UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
    FPacGridPosition CurrentGridPosition;

    UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
    FPacGridPosition TargetGridPosition;

    UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
    EPacDirection TravelDirection = EPacDirection::None;

    UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
    EPacDirection NextDirection = EPacDirection::None;

    UPROPERTY(EditDefaultsOnly, Category = "Grid Movement", meta = (ClampMin = "0.0"))
    float MovementSpeed = 250.0f;

    bool bMovementEnabled = true;

    float SpeedMultiplier = 1.0f;
};
