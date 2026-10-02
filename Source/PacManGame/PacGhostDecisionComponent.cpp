
#include "PacGhostDecisionComponent.h"

#include "PacGhostMovementComponent.h"
#include "PacMaze.h"

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

void UPacGhostDecisionComponent::HandleGridPositionChanged(FPacGridPosition const & Position)
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

void UPacGhostDecisionComponent::ChooseDirection()
{

    check(Maze);
    check(MovementComponent);

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
