
// clang-format off
#include "PacGhostDecisionComponent.h"
#include "PacGridMovementComponent.h"

#include "PacGhostMovementComponent.h"
#include "PacMaze.h"
// clang-format on

// helper function
namespace
{
EPacDirection GetOppositeDirection(EPacDirection Direction)
{
    switch (Direction)
    {
        case EPacDirection::Up:
            return EPacDirection::Down;

        case EPacDirection::Down:
            return EPacDirection::Up;

        case EPacDirection::Left:
            return EPacDirection::Right;

        case EPacDirection::Right:
            return EPacDirection::Left;

        default:
            return EPacDirection::None;
    }
}
}  // namespace

UPacGhostDecisionComponent::UPacGhostDecisionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPacGhostDecisionComponent::Initialize(APacMaze* InMaze,
                                            UPacGhostMovementComponent* InMovementComponent)
{
    check(InMaze);
    check(InMovementComponent);

    Maze              = InMaze;
    MovementComponent = InMovementComponent;

    MovementComponent->OnGridPositionChanged
        .AddUObject(this, &UPacGhostDecisionComponent::HandleGridPositionChanged);

    ChooseDirection();
}

void UPacGhostDecisionComponent::SetPlayerMovementComponent(
    UPacGridMovementComponent* InPlayerMovementComponent)
{
    if (PlayerMovementComponent == InPlayerMovementComponent)
    {
        return;
    }

    if (PlayerMovementComponent != nullptr)
    {
        PlayerMovementComponent->OnGridPositionChanged.RemoveAll(this);
    }

    PlayerMovementComponent = InPlayerMovementComponent;

    if (PlayerMovementComponent != nullptr)
    {
        PlayerMovementComponent->OnGridPositionChanged
            .AddUObject(this,
                        &UPacGhostDecisionComponent::HandlePlayerGridPositionChanged);
    }

    if (BehaviorMode == EPacGhostBehaviorMode::Chase)
    {
        ChooseDirection();
    }
}

void UPacGhostDecisionComponent::SetBlinkyMovementComponent(
    UPacGhostMovementComponent* InBlinkyMovementComponent)
{
    BlinkyMovementComponent = InBlinkyMovementComponent;
}

void UPacGhostDecisionComponent::HandlePlayerGridPositionChanged(
    [[maybe_unused]] FPacGridPosition const & Position /*unused parameter*/)
{
    if (BehaviorMode == EPacGhostBehaviorMode::Chase)
    {
        ChooseDirection();
    }
}

void UPacGhostDecisionComponent::SetScatterTarget(FPacGridPosition TargetPosition)
{
    if (!Maze->IsWalkable(TargetPosition))
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("PAC: Invalid Scatter target (%d,%d)."),
               TargetPosition.X,
               TargetPosition.Y);
        return;
    }
    if (Maze == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("PAC: Maze is null"));
        return;
    }

    ScatterTargetPosition = TargetPosition;
    bHasScatterTarget     = true;

    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: Scatter target set to (%d,%d)."),
           TargetPosition.X,
           TargetPosition.Y);

    if (BehaviorMode == EPacGhostBehaviorMode::Scatter)
    {
        ChooseDirection();
    }
}

void UPacGhostDecisionComponent::SetBehaviorMode(EPacGhostBehaviorMode NewMode)
{
    if (BehaviorMode == NewMode)
    {
        return;
    }

    bool const bWasScatterOrChase = BehaviorMode == EPacGhostBehaviorMode::Scatter ||
                                    BehaviorMode == EPacGhostBehaviorMode::Chase;

    bool const bIsScatterOrChase = NewMode == EPacGhostBehaviorMode::Scatter ||
                                   NewMode == EPacGhostBehaviorMode::Chase;

    bAllowReverseOnNextDecision = bWasScatterOrChase && bIsScatterOrChase;

    BehaviorMode = NewMode;

    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: Ghost behavior mode changed to %s"),
           *UEnum::GetValueAsString(BehaviorMode));

    ChooseDirection();

    bAllowReverseOnNextDecision = false;
}

void UPacGhostDecisionComponent::SetPersonality(EPacGhostPersonality NewPersonality)
{
    Personality = NewPersonality;

    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: Ghost personality set to %s"),
           *UEnum::GetValueAsString(Personality));
}

void UPacGhostDecisionComponent::HandleGridPositionChanged(
    [[maybe_unused]] FPacGridPosition const & Position)
{
    ChooseDirection();
}

bool UPacGhostDecisionComponent::IsDirectionWalkable(
    FPacGridPosition const & Position,
    EPacDirection const      Direction) const
{
    FPacGridPosition Neighbor;

    if (!Maze->TryGetNeighbor(Position, Direction, Neighbor))
    {
        return false;
    }

    return Maze->IsWalkable(Neighbor);
}

bool UPacGhostDecisionComponent::TryResolveWalkableTarget(
    FPacGridPosition const & DesiredTarget,
    FPacGridPosition &       OutTarget) const
{
    check(Maze);

    // The target is already valid and walkable.
    if (Maze->IsWalkable(DesiredTarget))
    {
        OutTarget = DesiredTarget;
        return true;
    }

    // The desired target is outside the grid or inside a wall.
    // Find the walkable tile closest to it using Manhattan distance.
    FIntPoint const GridSize = Maze->GetGridSize();

    int32 BestDistance = MAX_int32;
    bool  bFoundTarget = false;

    for (int32 Y = 0; Y < GridSize.Y; ++Y)
    {
        for (int32 X = 0; X < GridSize.X; ++X)
        {
            FPacGridPosition const Candidate(X, Y);

            if (!Maze->IsWalkable(Candidate))
            {
                continue;
            }

            int32 const Distance = FMath::Abs(X - DesiredTarget.X) +
                                   FMath::Abs(Y - DesiredTarget.Y);

            if (Distance < BestDistance)
            {
                BestDistance = Distance;
                OutTarget    = Candidate;
                bFoundTarget = true;
            }
        }
    }

    return bFoundTarget;
}

bool UPacGhostDecisionComponent::ChooseDirectionTowardTarget(
    FPacGridPosition const & TargetPosition)
{
    check(Maze);
    check(MovementComponent);

    FPacGridPosition const StartPosition = MovementComponent->GetCurrentGridPosition();

    FPacGridPosition ResolvedTargetPosition;

    if (!TryResolveWalkableTarget(TargetPosition, ResolvedTargetPosition))
    {
        UE_LOG(LogTemp, Error, TEXT("PAC: Could not resolve a walkable target."));

        return false;
    }


    // If we have reached the target, stop moving.
    if (StartPosition == ResolvedTargetPosition)
    {
        MovementComponent->SetDirection(EPacDirection::None);
        return true;
    }
    EPacDirection const Directions[] = {EPacDirection::Up,
                                        EPacDirection::Down,
                                        EPacDirection::Left,
                                        EPacDirection::Right};

    EPacDirection const CurrentDirection = MovementComponent->GetCurrentDirection();

    EPacDirection const OppositeDirection = GetOppositeDirection(CurrentDirection);

    // Try to find a path, optionally allowing an immediate reversal.
    auto FindPath = [&](bool bAllowReverse, EPacDirection & OutDirection) -> bool
    {
        TArray<FPacGridPosition>              Queue;
        TSet<FPacGridPosition>                Visited;
        TMap<FPacGridPosition, EPacDirection> FirstDirection;

        Queue.Add(StartPosition);
        Visited.Add(StartPosition);

        for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
        {
            FPacGridPosition const CurrentPosition = Queue[QueueIndex];

            for (EPacDirection const Direction : Directions)
            {
                FPacGridPosition NeighborPosition;

                if (!Maze->TryGetNeighbor(CurrentPosition, Direction, NeighborPosition))
                {
                    continue;
                }

                if (!Maze->IsWalkable(NeighborPosition) ||
                    Visited.Contains(NeighborPosition))
                {
                    continue;
                }

                // Only filter reversals on the first step.
                if (CurrentPosition == StartPosition && !bAllowReverse &&
                    Direction == OppositeDirection)
                {
                    continue;
                }

                Visited.Add(NeighborPosition);

                EPacDirection FirstStep = Direction;

                if (CurrentPosition != StartPosition)
                {
                    EPacDirection const* ExistingFirstStep = FirstDirection.Find(
                        CurrentPosition);

                    if (ExistingFirstStep == nullptr)
                    {
                        continue;
                    }

                    FirstStep = *ExistingFirstStep;
                }

                FirstDirection.Add(NeighborPosition, FirstStep);

                if (NeighborPosition == ResolvedTargetPosition)
                {
                    OutDirection = FirstStep;
                    return true;
                }

                Queue.Add(NeighborPosition);
            }
        }

        return false;
    };

    EPacDirection ChosenDirection = EPacDirection::None;

    // First prefer a route that doesn't reverse direction.
    if (FindPath(false, ChosenDirection))
    {
        MovementComponent->SetDirection(ChosenDirection);
        return true;
    }

    // On a mode switch, allow a reversal immediately.
    if (bAllowReverseOnNextDecision && FindPath(true, ChosenDirection))
    {
        MovementComponent->SetDirection(ChosenDirection);
        return true;
    }

    // Otherwise, allow a reversal only if no alternative route exists.
    if (FindPath(true, ChosenDirection))
    {
        MovementComponent->SetDirection(ChosenDirection);
        return true;
    }

    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: No path from (%d,%d) to target (%d,%d)."),
           StartPosition.X,
           StartPosition.Y,
           ResolvedTargetPosition.X,
           ResolvedTargetPosition.Y);

    return false;
}

void UPacGhostDecisionComponent::ChooseDirection()
{

    check(Maze);
    check(MovementComponent);

    if (BehaviorMode == EPacGhostBehaviorMode::Chase)
    {
        if (PlayerMovementComponent == nullptr)
        {
            UE_LOG(LogTemp,
                   Error,
                   TEXT("PAC: Chase mode has no PlayerMovementComponent."));
            return;
        }

        FPacGridPosition TargetPosition = PlayerMovementComponent->GetCurrentGridPosition();
        if (Personality == EPacGhostPersonality::Pinky)
        {
            EPacDirection const PlayerDirection = PlayerMovementComponent
                                                      ->GetCurrentDirection();

            if (PlayerDirection != EPacDirection::None)
            {
                // Look up to four walkable cells ahead of Pac-Man.
                for (int32 Step = 0; Step < 4; ++Step)
                {
                    FPacGridPosition const NextPosition = GetNeighbor(TargetPosition,
                                                                      PlayerDirection);

                    // Do not predict through a wall or beyond the maze.
                    if (!Maze->IsWalkable(NextPosition))
                    {
                        break;
                    }

                    TargetPosition = NextPosition;
                }
            }
        }
        else if (Personality == EPacGhostPersonality::Clyde)
        {
            FPacGridPosition const
                ClydePosition = MovementComponent->GetCurrentGridPosition();

            FPacGridPosition const PlayerPosition = PlayerMovementComponent
                                                        ->GetCurrentGridPosition();

            int32 const Distance = FMath::Abs(ClydePosition.X - PlayerPosition.X) +
                                   FMath::Abs(ClydePosition.Y - PlayerPosition.Y);

            if (Distance < 8 && bHasScatterTarget)
            {
                ChooseDirectionTowardTarget(ScatterTargetPosition);
                return;
            }
        }
        else if (Personality == EPacGhostPersonality::Inky)
        {
            if (BlinkyMovementComponent == nullptr)
            {
                UE_LOG(LogTemp,
                       Error,
                       TEXT("PAC: Inky has no BlinkyMovementComponent."));

                return;
            }

            // Start with Pac-Man's current position.
            FPacGridPosition PivotPosition = TargetPosition;

            // Move the pivot two tiles ahead of Pac-Man.
            EPacDirection const PlayerDirection = PlayerMovementComponent
                                                      ->GetCurrentDirection();

            if (PlayerDirection != EPacDirection::None)
            {
                FPacGridPosition const Offset = GetGridOffset(PlayerDirection);

                PivotPosition.X += Offset.X * 2;
                PivotPosition.Y += Offset.Y * 2;
            }

            // Read Blinky's current grid position.
            FPacGridPosition const BlinkyPosition = BlinkyMovementComponent
                                                        ->GetCurrentGridPosition();

            // Double the vector from Blinky to the pivot.
            TargetPosition.X = BlinkyPosition.X +
                               2 * (PivotPosition.X - BlinkyPosition.X);

            TargetPosition.Y = BlinkyPosition.Y +
                               2 * (PivotPosition.Y - BlinkyPosition.Y);
        }


        ChooseDirectionTowardTarget(TargetPosition);

        return;
    }
    if (BehaviorMode == EPacGhostBehaviorMode::Scatter && bHasScatterTarget)
    {
        ChooseDirectionTowardTarget(ScatterTargetPosition);
        return;
    }

    FPacGridPosition const CurrentPosition = MovementComponent->GetCurrentGridPosition();

    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: Ghost ChooseDirection at (%d,%d)"),
           CurrentPosition.X,
           CurrentPosition.Y);

    TArray<EPacDirection> ValidDirections;

    EPacDirection const Directions[] = {EPacDirection::Up,
                                        EPacDirection::Down,
                                        EPacDirection::Left,
                                        EPacDirection::Right};

    for (EPacDirection const Direction : Directions)
    {
        if (IsDirectionWalkable(CurrentPosition, Direction))
        {
            ValidDirections.Add(Direction);
        }
    }

    if (ValidDirections.IsEmpty())
    {
        return;
    }

    int32 const RandomIndex = FMath::RandRange(0, ValidDirections.Num() - 1);
    EPacDirection const ChosenDirection = ValidDirections[RandomIndex];

    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: Ghost chose direction %d from %d valid directions."),
           static_cast<uint8>(ChosenDirection),
           ValidDirections.Num());

    MovementComponent->SetDirection(ChosenDirection);
}
