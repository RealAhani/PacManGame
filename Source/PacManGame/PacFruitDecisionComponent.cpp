// clang-format off
#include "PacFruitDecisionComponent.h"
#include "Logging/PacManLog.h"
#include "PacGhostMovementComponent.h"
#include "PacGridMovementComponent.h"
#include "PacMaze.h"
// clang-format on

namespace
{
EPacDirection GetOpposite(EPacDirection const Direction)
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

UPacFruitDecisionComponent::UPacFruitDecisionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPacFruitDecisionComponent::Initialize(
    APacMaze*                   InMaze,
    UPacGhostMovementComponent* InMovementComponent,
    UPacGridMovementComponent*  InPlayerMovementComponent)
{
    check(InMaze);
    check(InMovementComponent);

    Maze                    = InMaze;
    MovementComponent       = InMovementComponent;
    PlayerMovementComponent = InPlayerMovementComponent;

    // Never bind twice, even if Initialize() is called again.
    MovementComponent->OnGridPositionChanged.RemoveAll(this);
    MovementComponent->OnGridPositionChanged
        .AddUObject(this, &UPacFruitDecisionComponent::HandleGridPositionChanged);
}

void UPacFruitDecisionComponent::Respawn()
{
    if (Maze == nullptr || MovementComponent == nullptr)
    {
        return;
    }

    ChooseDirection();
}

void UPacFruitDecisionComponent::HandleGridPositionChanged(
    [[maybe_unused]] FPacGridPosition const & Position)
{
    ChooseDirection();
}

void UPacFruitDecisionComponent::ChooseDirection()
{
    check(Maze);
    check(MovementComponent);

    FPacGridPosition const Current = MovementComponent->GetCurrentGridPosition();
    EPacDirection const Opposite = GetOpposite(
        MovementComponent->GetTravelDirection());

    if (PlayerMovementComponent != nullptr)
    {
        FPacGrid const & Grid = Maze->GetGrid();

        FPacGridPosition Target;

        if (Grid.FindFleeTarget(Current,
                                PlayerMovementComponent->GetCurrentGridPosition(),
                                Target))
        {
            EPacDirection Step = EPacDirection::None;

            // First try without turning back; in a dead end allow it.
            if (Grid.FindFirstStep(Current, Target, Opposite, Step) ||
                Grid.FindFirstStep(Current, Target, EPacDirection::None, Step))
            {
                MovementComponent->SetDirection(Step);
                return;
            }
        }
    }

    // Cornered, or no route: wander.
    ChooseRandomDirection();
}

bool UPacFruitDecisionComponent::ChooseRandomDirection()
{
    FPacGridPosition const Current = MovementComponent->GetCurrentGridPosition();
    EPacDirection const Opposite = GetOpposite(
        MovementComponent->GetTravelDirection());

    EPacDirection const Directions[] = {EPacDirection::Up,
                                        EPacDirection::Down,
                                        EPacDirection::Left,
                                        EPacDirection::Right};

    TArray<EPacDirection> Options;
    bool                  bCanReverse = false;

    for (EPacDirection const Direction : Directions)
    {
        FPacGridPosition Next;

        // The fruit never enters the ghost house (IsWalkable default).
        if (!Maze->TryGetNeighborWrapping(Current, Direction, Next) ||
            !Maze->IsWalkable(Next))
        {
            continue;
        }

        if (Direction == Opposite)
        {
            bCanReverse = true;
            continue;
        }

        Options.Add(Direction);
    }

    if (Options.IsEmpty() && bCanReverse)
    {
        Options.Add(Opposite);
    }

    if (Options.IsEmpty())
    {
        return false;
    }

    MovementComponent->SetDirection(Options[FMath::RandRange(0, Options.Num() - 1)]);
    return true;
}
