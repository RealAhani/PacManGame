// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacGridMovementComponent.h"
#include "PacMaze.h"
// clang-format on

UPacGridMovementComponent::UPacGridMovementComponent()
{
    PrimaryComponentTick.bCanEverTick          = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    bAutoActivate = true;
}

void UPacGridMovementComponent::TickComponent(
    float                        DeltaTime,
    ELevelTick                   TickType,
    FActorComponentTickFunction* ThisTickFunction)
{

    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (Maze == nullptr || UpdatedComponent == nullptr)
    {
        if (Maze == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("PAC: maze is null"));
        }
        if (UpdatedComponent == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("PAC: updated component is null"));
        }

        return;
    }
    // Spend this frame's whole movement budget. When a tile is reached with
    // distance left over, start the next tile right away instead of dropping
    // the leftover and waiting for the next frame (that caused a speed hitch
    // at every tile).
    float RemainingDistance = MovementSpeed * DeltaTime;

    // Safety cap so a huge DeltaTime (hitch, breakpoint) cannot loop for long.
    constexpr int32 MaxTilesPerFrame = 4;

    for (int32 Step = 0;
         Step < MaxTilesPerFrame && RemainingDistance > KINDA_SMALL_NUMBER;
         ++Step)
    {
        if (CurrentGridPosition == TargetGridPosition && !TryStartMovement())
        {
            break;  // Blocked or no input: stand still, drop the budget.
        }

        RemainingDistance = MoveTowardTarget(RemainingDistance);
    }
}

void UPacGridMovementComponent::SetDesiredDirection(EPacDirection const Direction)
{

    if (Direction == EPacDirection::None)
    {
        return;
    }

    // If Pac-Man is actually stopped by a wall,
    // a new input starts a completely new command sequence.
    if (bMovementBlocked)
    {
        InputQueue.Reset();
        bMovementBlocked = false;
    }

    InputQueue.Add(Direction);

    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: Input received. Direction=%d, QueueSize=%d"),
           static_cast<uint8>(Direction),
           InputQueue.Num());
}

FPacGridPosition UPacGridMovementComponent::GetCurrentGridPosition() const
{
    return CurrentGridPosition;
}

EPacDirection UPacGridMovementComponent::GetCurrentDirection() const
{
    return CurrentDirection;
}

bool UPacGridMovementComponent::TryStartMovement()
{

    // -------------------------------------------------
    // 1. Try the oldest requested direction first.
    // -------------------------------------------------

    if (!InputQueue.IsEmpty())
    {

        EPacDirection const RequestedDirection = InputQueue[0];

        FPacGridPosition RequestedNextPosition;

        if (Maze->TryGetNeighbor(CurrentGridPosition,
                                 RequestedDirection,
                                 RequestedNextPosition) &&
            Maze->IsWalkable(RequestedNextPosition))
        {

            // Requested direction is available.
            CurrentDirection   = RequestedDirection;
            TargetGridPosition = RequestedNextPosition;

            // Remove only after successfully starting movement.
            InputQueue.RemoveAt(0);

            bMovementBlocked = false;

            // UE_LOG(LogTemp,
            //        Warning,
            //        TEXT("PAC: Requested direction accepted. "
            //             "Current=(%d,%d), Target=(%d,%d), Direction=%d, "
            //             "Remaining=%d"),
            //        CurrentGridPosition.X,
            //        CurrentGridPosition.Y,
            //        TargetGridPosition.X,
            //        TargetGridPosition.Y,
            //        static_cast<uint8>(CurrentDirection),
            //        InputQueue.Num());

            return true;
        }

        // IMPORTANT:
        // Requested direction is blocked RIGHT NOW.
        //
        // Do NOT remove it.
        // Do NOT set bMovementBlocked.
        //
        // We continue below and try CurrentDirection.
        // UE_LOG(LogTemp,
        //        Warning,
        //        TEXT("PAC: Requested direction blocked for now. "
        //             "Keeping it in queue. Direction=%d"),
        //        static_cast<uint8>(RequestedDirection));
    }

    // -------------------------------------------------
    // 2. No usable requested direction.
    //    Continue in the current direction.
    // -------------------------------------------------

    if (CurrentDirection == EPacDirection::None)
    {
        bMovementBlocked = !InputQueue.IsEmpty();

        return false;
    }

    FPacGridPosition CurrentNextPosition;

    if (!Maze->TryGetNeighbor(CurrentGridPosition, CurrentDirection, CurrentNextPosition))
    {

        bMovementBlocked = true;

        // UE_LOG(LogTemp,
        //        Warning,
        //        TEXT("PAC: Current direction has no neighbor. "
        //             "Pac-Man is stopped."));

        return false;
    }

    if (!Maze->IsWalkable(CurrentNextPosition))
    {

        bMovementBlocked = true;

        // UE_LOG(LogTemp,
        //        Warning,
        //        TEXT("PAC: Current direction is blocked. "
        //             "Pac-Man is stopped at (%d,%d)."),
        //        CurrentGridPosition.X,
        //        CurrentGridPosition.Y);

        return false;
    }

    // Current direction is still available.
    TargetGridPosition = CurrentNextPosition;

    bMovementBlocked = false;

    return true;
}

float UPacGridMovementComponent::MoveTowardTarget(float const MaxDistance)
{
    check(Maze);
    check(UpdatedComponent);

    FVector const TargetWorldPosition = Maze->GridToWorld(TargetGridPosition);

    FVector const CurrentWorldPosition = UpdatedComponent->GetComponentLocation();

    FVector const ToTarget = TargetWorldPosition - CurrentWorldPosition;

    float const DistanceToTarget = ToTarget.Size();

    if (DistanceToTarget <= KINDA_SMALL_NUMBER || MaxDistance >= DistanceToTarget)
    {
        // Arrive exactly on the tile centre and hand back what is left.
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

void UPacGridMovementComponent::InitializeMaze(APacMaze* InMaze)
{
    check(InMaze);

    Maze = InMaze;

    APawn* OwnerPawn = GetPawnOwner();

    check(OwnerPawn);

    CurrentGridPosition = Maze->GetPlayerStart();
    TargetGridPosition  = CurrentGridPosition;

    InputQueue.Reset();
    CurrentDirection = EPacDirection::None;
    bMovementBlocked = false;

    FVector const StartWorldPosition = Maze->GridToWorld(CurrentGridPosition);

    OwnerPawn->SetActorLocation(StartWorldPosition);

    // SetComponentTickEnabled(true);

    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: Movement initialized. "
                "Grid=(%d,%d), Tick=%s, Maze=%s"),
           CurrentGridPosition.X,
           CurrentGridPosition.Y,
           IsComponentTickEnabled() ? TEXT("Enabled") : TEXT("Disabled"),
           *GetNameSafe(Maze));
}
