// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "PacGridTypes.h"
#include "PacGridMovementComponent.generated.h"
// clang-format on

class APacMaze;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGridPositionChanged, FPacGridPosition const &);

UCLASS(ClassGroup = (PacMan), meta = (BlueprintSpawnableComponent))
class PACMANGAME_API UPacGridMovementComponent : public UPawnMovementComponent
{
    GENERATED_BODY()

public:

    // -------------------- functions --------------------
    UPacGridMovementComponent();

    void SetDesiredDirection(EPacDirection Direction);

    FPacGridPosition GetCurrentGridPosition() const;
    EPacDirection    GetCurrentDirection() const;

    void InitializeMaze(APacMaze* InMaze);
    // Freezes / unfreezes movement (death pause, level complete, game over).
    void SetMovementEnabled(bool bEnabled);

    bool IsMovementBlocked() const
    {
        return bMovementBlocked;
    }


    // -------------------- properties --------------------
    FOnGridPositionChanged OnGridPositionChanged;

protected:

    virtual void TickComponent(float      DeltaTime,
                               ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:

    // -------------------- functions --------------------
    bool TryStartMovement();
    // Moves up to MaxDistance toward the target tile. Returns the distance
    // that was NOT used (greater than zero only if the target was reached),
    // so the caller can carry it into the next tile.
    float MoveTowardTarget(float MaxDistance);

    // True while the current move is a tunnel step and Pac-Man / the ghost has
    // not yet left the map on its way to the other side.
    bool bCrossingTunnel = false;


    // -------------------- properties --------------------
    UPROPERTY(EditInstanceOnly, Category = "Grid Movement")
    TObjectPtr<APacMaze> Maze;

    UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
    FPacGridPosition CurrentGridPosition;

    UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
    EPacDirection CurrentDirection = EPacDirection::None;

    UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
    TArray<EPacDirection> InputQueue;

    UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
    FPacGridPosition TargetGridPosition;

    UPROPERTY(EditDefaultsOnly, Category = "Grid Movement", meta = (ClampMin = "0.0"))
    float MovementSpeed = 300.0f;

    bool bMovementBlocked = false;
    bool bMovementEnabled = true;
};
