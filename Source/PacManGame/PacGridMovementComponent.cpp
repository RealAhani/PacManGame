// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacGridMovementComponent.h"
#include "Logging/PacManLog.h"
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

    // Not initialised yet (InitializeMaze() has not run). That is a normal
    // state for the first frames, so stay silent instead of spamming per tick.
    if (!bMovementEnabled || Maze == nullptr || UpdatedComponent == nullptr)
    {
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

    UE_LOG(LogPacMan,
           Verbose,
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

        if (Maze->TryGetNeighborWrapping(CurrentGridPosition,
                                         RequestedDirection,
                                         RequestedNextPosition) &&
            Maze->IsWalkable(RequestedNextPosition))
        {

            // Requested direction is available.
            CurrentDirection   = RequestedDirection;
            TargetGridPosition = RequestedNextPosition;
            bCrossingTunnel    = FPacGrid::IsWrapStep(CurrentGridPosition,
                                                      TargetGridPosition);


            // Remove only after successfully starting movement.
            InputQueue.RemoveAt(0);

            bMovementBlocked = false;

            return true;
        }

        // IMPORTANT:
        // Requested direction is blocked RIGHT NOW.
        //
        // Do NOT remove it.
        // Do NOT set bMovementBlocked.
        //
        // We continue below and try CurrentDirection.
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

    if (!Maze->TryGetNeighborWrapping(CurrentGridPosition,
                                      CurrentDirection,
                                      CurrentNextPosition))
    {

        bMovementBlocked = true;


        return false;
    }

    if (!Maze->IsWalkable(CurrentNextPosition))
    {

        bMovementBlocked = true;

        return false;
    }

    // Current direction is still available.
    TargetGridPosition = CurrentNextPosition;
    bCrossingTunnel = FPacGrid::IsWrapStep(CurrentGridPosition, TargetGridPosition);

    bMovementBlocked = false;

    return true;
}

float UPacGridMovementComponent::MoveTowardTarget(float const MaxDistance)
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

void UPacGridMovementComponent::InitializeMaze(APacMaze* InMaze)
{
    check(InMaze);

    Maze = InMaze;

    APawn* OwnerPawn = GetPawnOwner();

    check(OwnerPawn);

    CurrentGridPosition = Maze->GetPlayerStart();
    TargetGridPosition  = CurrentGridPosition;
    bCrossingTunnel     = false;

    InputQueue.Reset();
    CurrentDirection = EPacDirection::None;
    bMovementBlocked = false;

    FVector const StartWorldPosition = Maze->GridToWorld(CurrentGridPosition);

    OwnerPawn->SetActorLocation(StartWorldPosition);

    // SetComponentTickEnabled(true);

    UE_LOG(LogPacMan,
           Log,
           TEXT("PAC: Movement initialized. "
                "Grid=(%d,%d), Tick=%s, Maze=%s"),
           CurrentGridPosition.X,
           CurrentGridPosition.Y,
           IsComponentTickEnabled() ? TEXT("Enabled") : TEXT("Disabled"),
           *GetNameSafe(Maze));
}

void UPacGridMovementComponent::SetMovementEnabled(bool const bEnabled)
{
    bMovementEnabled = bEnabled;
}
