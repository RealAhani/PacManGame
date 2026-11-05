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

    if (!bMovementEnabled || Maze == nullptr || UpdatedComponent == nullptr)
    {
        return;
    }
    // Spend this frame's whole movement budget, carrying leftover distance
    // into the next tile. A ghost that arrives and decides a new direction
    // (via OnGridPositionChanged) starts moving in the same frame.
    float RemainingDistance = MovementSpeed * SpeedMultiplier * DeltaTime;

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

    check(Maze->IsWalkable(StartPosition, true));

    CurrentGridPosition = StartPosition;
    TargetGridPosition  = StartPosition;
    bCrossingTunnel     = false;
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

    if (!Maze->TryGetNeighborWrapping(CurrentGridPosition, NextDirection, NextPosition))
    {
        return false;
    }

    // Even if a stale direction is left over, a ghost cannot step into the
    // house unless it is allowed to.
    if (!Maze->IsWalkable(NextPosition, bCanEnterGhostHouse))
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
    bCrossingTunnel = FPacGrid::IsWrapStep(CurrentGridPosition, TargetGridPosition);

    return true;
}

float UPacGhostMovementComponent::MoveTowardTarget(float const MaxDistance)
{
    check(Maze);
    check(UpdatedComponent);

    // A tunnel step is two walks: first off the edge of the map to a virtual
    // tile just outside it, then (after popping out on the opposite side) from
    // the virtual tile there into the real target tile.
    FPacGridPosition const ExitTile(CurrentGridPosition.X, CurrentGridPosition.Y);

    FVector const GoalWorldPosition = bCrossingTunnel
                                          ? Maze->GridToWorldUnchecked(ExitTile)
                                          : Maze->GridToWorld(TargetGridPosition);

    FVector const CurrentWorldPosition = UpdatedComponent->GetComponentLocation();

    FVector const ToGoal = GoalWorldPosition - CurrentWorldPosition;

    float const DistanceToGoal = ToGoal.Size();


    if (DistanceToGoal <= KINDA_SMALL_NUMBER || MaxDistance >= DistanceToGoal)
    {
        float const LeftOver = FMath::Max(MaxDistance - DistanceToGoal, 0.0f);

        if (bCrossingTunnel)
        {
            // Left the map. Reappear one tile outside the opposite edge and
            // carry the leftover distance into the walk to the target tile.
            bCrossingTunnel = false;

            FPacGridPosition const EntryTile(TargetGridPosition.X,
                                             TargetGridPosition.Y);

            UpdatedComponent->SetWorldLocation(
                Maze->GridToWorldUnchecked(EntryTile));

            return LeftOver;
        }

        // Arrive exactly on the tile centre and hand back what is left.
        UpdatedComponent->SetWorldLocation(GoalWorldPosition);

        CurrentGridPosition = TargetGridPosition;
        OnGridPositionChanged.Broadcast(CurrentGridPosition);

        return LeftOver;
    }

    FVector const MovementDelta = ToGoal.GetSafeNormal() * MaxDistance;

    FHitResult Hit;

    SafeMoveUpdatedComponent(MovementDelta,
                             UpdatedComponent->GetComponentQuat(),
                             false,
                             Hit,
                             ETeleportType::None);
    return 0.0f;
}

void UPacGhostMovementComponent::SetMovementEnabled(bool const bEnabled)
{
    bMovementEnabled = bEnabled;
}

void UPacGhostMovementComponent::SetSpeedMultiplier(float const Multiplier)
{
    SpeedMultiplier = FMath::Max(Multiplier, 0.0f);
}

void UPacGhostMovementComponent::SetCanEnterGhostHouse(bool const bCanEnter)
{
    bCanEnterGhostHouse = bCanEnter;
}
