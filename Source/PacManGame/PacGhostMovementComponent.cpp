// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacGhostMovementComponent.h"
#include "PacMaze.h"

#include "Logging/PacManLog.h"
// clang-format on

UPacGhostMovementComponent::UPacGhostMovementComponent()
{
    PrimaryComponentTick.bCanEverTick          = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    bAutoActivate                              = true;
}

void UPacGhostMovementComponent::TickComponent(
    float                        DeltaTime,
    ELevelTick                   TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (Maze == nullptr || UpdatedComponent == nullptr)
    {
        return;
    }
    // Spend this frame's whole movement budget, carrying leftover distance
    // into the next tile. A ghost that arrives and decides a new direction
    // (via OnGridPositionChanged) starts moving in the same frame.
    float RemainingDistance = MovementSpeed * DeltaTime;

    // Safety cap so a huge DeltaTime (hitch, breakpoint) cannot loop for long.
    constexpr int32 MaxTilesPerFrame = 4;

    for (int32 Step = 0;
         Step < MaxTilesPerFrame && RemainingDistance > KINDA_SMALL_NUMBER;
         ++Step)
    {
        if (CurrentGridPosition == TargetGridPosition && !TryStartMovement())
        {
            break;  // No usable direction: stand still, drop the budget.
        }

        RemainingDistance = MoveTowardTarget(RemainingDistance);
    }
}

void UPacGhostMovementComponent::SetDirection(EPacDirection const Direction)
{
    UE_LOG(LogPacMan,
           Verbose,
           TEXT("PAC: Ghost SetDirection = %s"),
           *UEnum::GetValueAsString(Direction));

    NextDirection = Direction;
}

FPacGridPosition UPacGhostMovementComponent::GetCurrentGridPosition() const
{
    return CurrentGridPosition;
}

EPacDirection UPacGhostMovementComponent::GetTravelDirection() const
{
    return TravelDirection;
}

void UPacGhostMovementComponent::InitializeMaze(APacMaze* InMaze,
                                                FPacGridPosition const & StartPosition)
{
    check(InMaze);

    Maze = InMaze;

    check(Maze->IsWalkable(StartPosition));

    CurrentGridPosition = StartPosition;
    TargetGridPosition  = StartPosition;
    TravelDirection     = EPacDirection::None;
    NextDirection       = EPacDirection::None;

    APawn* OwnerPawn = GetPawnOwner();
    check(OwnerPawn);

    OwnerPawn->SetActorLocation(Maze->GridToWorld(CurrentGridPosition));
}

bool UPacGhostMovementComponent::TryStartMovement()
{
    UE_LOG(LogPacMan,
           Verbose,
           TEXT("PAC: Ghost TryStartMovement.Position=(%d,%d),Direction=%d"),
           CurrentGridPosition.X,
           CurrentGridPosition.Y,
           static_cast<uint8>(NextDirection));

    if (NextDirection == EPacDirection::None)
    {
        return false;
    }

    FPacGridPosition NextPosition;

    if (!Maze->TryGetNeighbor(CurrentGridPosition, NextDirection, NextPosition))
    {
        return false;
    }

    if (!Maze->IsWalkable(NextPosition))
    {
        return false;
    }

    UE_LOG(LogPacMan,
           Verbose,
           TEXT("PAC: Ghost starting movement to (%d,%d)."),
           NextPosition.X,
           NextPosition.Y);

    // The move starts here, so this is the only place the travel direction
    // changes. Decisions made mid-move cannot corrupt it.
    TravelDirection    = NextDirection;
    TargetGridPosition = NextPosition;

    return true;
}

float UPacGhostMovementComponent::MoveTowardTarget(float const MaxDistance)
{
    check(Maze);
    check(UpdatedComponent);

    FVector const TargetWorldPosition = Maze->GridToWorld(TargetGridPosition);

    FVector const CurrentWorldPosition = UpdatedComponent->GetComponentLocation();

    FVector const ToTarget = TargetWorldPosition - CurrentWorldPosition;

    float const DistanceToTarget = ToTarget.Size();


    if (DistanceToTarget <= KINDA_SMALL_NUMBER || MaxDistance >= DistanceToTarget)
    {
        // Array exactly on the tile centre and hand back what is left.
        UpdatedComponent->SetWorldLocation(TargetWorldPosition);

        CurrentGridPosition = TargetGridPosition;
        OnGridPositionChanged.Broadcast(CurrentGridPosition);

        return FMath::Max(MaxDistance - DistanceToTarget, 0.0f);
    }

    FVector const MovementDelta = ToTarget.GetSafeNormal() * MaxDistance;

    FHitResult Hit;

    SafeMoveUpdatedComponent(MovementDelta,
                             UpdatedComponent->GetComponentQuat(),
                             false,
                             Hit,
                             ETeleportType::None);
    return 0.0f;
}
