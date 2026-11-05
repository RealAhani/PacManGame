
// clang-format off
#include "PacGhostDecisionComponent.h"
#include "PacGridMovementComponent.h"
#include "PacGhostMovementComponent.h"
#include "PacMaze.h"
#include "Logging/PacManLog.h"
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

    UpdateHousePermission();

    ChooseDirection();
}

void UPacGhostDecisionComponent::ConfigureHouse(bool const bStartInHouse,
                                                FPacGridPosition const ExitPosition)
{
    HouseState        = bStartInHouse ? EPacGhostHouseState::InHouse
                                      : EPacGhostHouseState::Outside;
    HouseExitPosition = ExitPosition;

    UpdateHousePermission();
}

void UPacGhostDecisionComponent::StartLeavingHouse()
{
    HouseState = EPacGhostHouseState::Leaving;
    UpdateHousePermission();

    UE_LOG(LogPacMan, Log, TEXT("PAC: Ghost leaves the house."));

    if (Maze != nullptr && MovementComponent != nullptr &&
        !MovementComponent->IsMoving())
    {
        ChooseDirection();
    }
}

void UPacGhostDecisionComponent::ResetWaveTracking()
{
    LastRequestedHuntMode = EPacGhostBehaviorMode::None;
}

// The house door is closed to a ghost unless it is inside / leaving, or it was
// eaten and is running home.
bool UPacGhostDecisionComponent::CanEnterHouse() const
{
    return HouseState != EPacGhostHouseState::Outside ||
           BehaviorMode == EPacGhostBehaviorMode::Eaten;
}

void UPacGhostDecisionComponent::UpdateHousePermission()
{
    if (MovementComponent != nullptr)
    {
        MovementComponent->SetCanEnterGhostHouse(CanEnterHouse());
    }
}


void UPacGhostDecisionComponent::Respawn()
{
    check(Maze);
    check(MovementComponent);

    bAllowReverseOnNextDecision = false;
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
    //    While the ghost is between tiles, the decision is made on arrival
    // (HandleGridPositionChanged) using the latest player position, so
    // deciding now would only be thrown away. Only re-decide a ghost that
    // is standing still.
    if (BehaviorMode == EPacGhostBehaviorMode::Chase &&
        MovementComponent != nullptr && !MovementComponent->IsMoving())
    {
        ChooseDirection();
    }
}

void UPacGhostDecisionComponent::SetScatterTarget(FPacGridPosition TargetPosition)
{
    if (Maze == nullptr)
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: Maze is null"));
        return;
    }
    if (!Maze->IsWalkable(TargetPosition))
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: Invalid Scatter target (%d,%d)."),
               TargetPosition.X,
               TargetPosition.Y);
        return;
    }

    ScatterTargetPosition = TargetPosition;
    bHasScatterTarget     = true;

    UE_LOG(LogPacMan,
           Log,
           TEXT("PAC: Scatter target set to (%d,%d)."),
           TargetPosition.X,
           TargetPosition.Y);

    if (BehaviorMode == EPacGhostBehaviorMode::Scatter)
    {
        ChooseDirection();
    }
}

void UPacGhostDecisionComponent::SetBehaviorMode(EPacGhostBehaviorMode const NewMode)
{
    bool const bHuntingMode = NewMode == EPacGhostBehaviorMode::Scatter ||
                              NewMode == EPacGhostBehaviorMode::Chase;

    if (bHuntingMode)
    {
        BaseMode = NewMode;

        // A Scatter request after something else (or after nothing) is the
        // start of a new wave.
        if (NewMode == EPacGhostBehaviorMode::Scatter &&
            LastRequestedHuntMode != EPacGhostBehaviorMode::Scatter)
        {
            LastRequestedHuntMode = NewMode;
            OnScatterRoundStarted.Broadcast();
        }

        LastRequestedHuntMode = NewMode;


        // Frightened / Eaten win over the schedule. The new base mode is
        // remembered and applied by ReleaseOverride().
        if (BehaviorMode == EPacGhostBehaviorMode::Frightened ||
            BehaviorMode == EPacGhostBehaviorMode::Eaten)
        {
            UE_LOG(LogPacMan,
                   Verbose,
                   TEXT("PAC: Base mode %s remembered, ghost is %s."),
                   *UEnum::GetValueAsString(NewMode),
                   *UEnum::GetValueAsString(BehaviorMode));
            return;
        }
    }

    ApplyMode(NewMode);
}

void UPacGhostDecisionComponent::ReleaseOverride()
{
    ApplyMode(BaseMode);
}

void UPacGhostDecisionComponent::ApplyMode(EPacGhostBehaviorMode const NewMode)
{
    if (BehaviorMode == NewMode)
    {
        return;
    }

    bool const bWasScatterOrChase = BehaviorMode == EPacGhostBehaviorMode::Scatter ||
                                    BehaviorMode == EPacGhostBehaviorMode::Chase;

    bool const bIsScatterOrChase = NewMode == EPacGhostBehaviorMode::Scatter ||
                                   NewMode == EPacGhostBehaviorMode::Chase;


    // Classic rule: switching Scatter <-> Chase, or getting frightened, lets
    // the ghost turn around once.
    bAllowReverseOnNextDecision = bWasScatterOrChase &&
                                  (bIsScatterOrChase ||
                                   NewMode == EPacGhostBehaviorMode::Frightened);

    BehaviorMode = NewMode;

    float SpeedMultiplier = 1.0f;
    if (NewMode == EPacGhostBehaviorMode::Frightened)
    {
        SpeedMultiplier = FrightenedSpeedMultiplier;
    }
    else if (NewMode == EPacGhostBehaviorMode::Eaten)
    {
        SpeedMultiplier = EatenSpeedMultiplier;
    }
    MovementComponent->SetSpeedMultiplier(SpeedMultiplier);
    UpdateHousePermission();

    OnModeChanged.Broadcast(NewMode);


    UE_LOG(LogPacMan,
           Warning,
           TEXT("PAC: Ghost behavior mode changed to %s"),
           *UEnum::GetValueAsString(BehaviorMode));

    // A mid-move switch waits for the next tile. The reversal permission
    // stays set until HandleGridPositionChanged() consumes it.
    if (!MovementComponent->IsMoving())
    {
        ChooseDirection();
        bAllowReverseOnNextDecision = false;
    }
}

void UPacGhostDecisionComponent::SetHomePosition(FPacGridPosition const InHomePosition)
{
    HomePosition     = InHomePosition;
    bHasHomePosition = true;
}

void UPacGhostDecisionComponent::SetPersonality(EPacGhostPersonality NewPersonality)
{
    Personality = NewPersonality;

    UE_LOG(LogPacMan,
           Log,
           TEXT("PAC: Ghost personality set to %s"),
           *UEnum::GetValueAsString(Personality));
}

void UPacGhostDecisionComponent::HandleGridPositionChanged(
    [[maybe_unused]] FPacGridPosition const & Position)
{
    // Out of the door: from now on the house is closed to this ghost again.
    if (HouseState == EPacGhostHouseState::Leaving && Position == HouseExitPosition)
    {
        HouseState = EPacGhostHouseState::Outside;
        UpdateHousePermission();
    }

    // The ghost just arrived on a tile: this is the real decision point.
    ChooseDirection();
    // A pending "reverse after mode switch" permission is used up once.
    bAllowReverseOnNextDecision = false;
}

bool UPacGhostDecisionComponent::IsDirectionWalkable(
    FPacGridPosition const & Position,
    EPacDirection const      Direction) const
{
    FPacGridPosition Neighbor;

    if (!Maze->TryGetNeighborWrapping(Position, Direction, Neighbor))
    {
        return false;
    }

    return Maze->IsWalkable(Neighbor, CanEnterHouse());
}


bool UPacGhostDecisionComponent::ChooseDirectionTowardTarget(
    FPacGridPosition const & TargetPosition)
{
    check(Maze);
    check(MovementComponent);
    // All maze questions are answered by the plain-data grid (see
    // Grid/PacGrid.h), which is unit tested without a world.
    FPacGrid const & Grid = Maze->GetGrid();

    // The desired target may be inside a wall or off the grid (for example
    // Inky's doubled vector). Use the closest walkable tile instead.
    FPacGridPosition const StartPosition = MovementComponent->GetCurrentGridPosition();

    FPacGridPosition ResolvedTargetPosition;

    bool const bAllowHouse = CanEnterHouse();

    if (!Grid.FindNearestWalkable(TargetPosition, ResolvedTargetPosition, bAllowHouse))
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: Could not resolve a walkable target."));

        return false;
    }


    // Standing on the target (a scatter corner, or Pac-Man's tile): keep
    // moving instead of parking. Because the ghost may not reverse, the next
    // decision routes it back around the target, so it circles the corner.
    if (StartPosition == ResolvedTargetPosition)
    {
        return ChooseRandomDirection();
    }

    // Use the direction the ghost really travelled to get here, not the last
    // decision that was stored.
    // A pending mode switch (Scatter <-> Chase) lifts the no-reverse rule once.
    EPacDirection const
        ForbiddenFirstStep = bAllowReverseOnNextDecision
                                 ? EPacDirection::None
                                 : GetOppositeDirection(
                                       MovementComponent->GetTravelDirection());

    EPacDirection ChosenDirection = EPacDirection::None;

    // Prefer a route that doesn't start by reversing. If every route needs a
    // reversal (dead end), allow it.
    if (Grid.FindFirstStep(StartPosition,
                           ResolvedTargetPosition,
                           ForbiddenFirstStep,
                           ChosenDirection,
                           bAllowHouse) ||
        (ForbiddenFirstStep != EPacDirection::None &&
         Grid.FindFirstStep(StartPosition,
                            ResolvedTargetPosition,
                            EPacDirection::None,
                            ChosenDirection,
                            bAllowHouse)))
    {
        MovementComponent->SetDirection(ChosenDirection);
        return true;
    }

    UE_LOG(LogPacMan,
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
    // The house comes first: a waiting ghost wanders inside, a leaving ghost
    // walks out, whatever its Scatter / Chase / Frightened mode is.
    if (HouseState == EPacGhostHouseState::InHouse)
    {
        ChooseDirectionInHouse();
        return;
    }

    if (HouseState == EPacGhostHouseState::Leaving)
    {
        ChooseDirectionTowardTarget(HouseExitPosition);
        return;
    }


    if (BehaviorMode == EPacGhostBehaviorMode::Chase)
    {
        if (PlayerMovementComponent == nullptr)
        {
            UE_LOG(LogPacMan,
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

    // An eaten ghost runs home by the shortest way.
    if (BehaviorMode == EPacGhostBehaviorMode::Eaten && bHasHomePosition)
    {
        ChooseDirectionTowardTarget(HomePosition);
        return;
    }

    // Frightened (and anything without a target) wanders.
    ChooseRandomDirection();
}

// Waiting in the house: wander randomly. Only house tiles count, so the ghost
// can never walk out of the door by itself.
bool UPacGhostDecisionComponent::ChooseDirectionInHouse()
{
    check(Maze);
    check(MovementComponent);

    FPacGridPosition const Current = MovementComponent->GetCurrentGridPosition();

    auto const CollectHouseDirections =
        [this, &Current](TArray<EPacDirection> & Out,
                         EPacDirection const     First,
                         EPacDirection const     Second)
    {
        for (EPacDirection const Direction : {First, Second})
        {
            if (Maze->IsGhostHouseTile(GetNeighbor(Current, Direction)))
            {
                Out.Add(Direction);
            }
        }
    };

    TArray<EPacDirection> Options;
    CollectHouseDirections(Options, EPacDirection::Up, EPacDirection::Down);

    if (Options.IsEmpty())
    {
        CollectHouseDirections(Options, EPacDirection::Left, EPacDirection::Right);
    }

    if (Options.IsEmpty())
    {
        return false;  // a one-tile house: just stand there
    }

    MovementComponent->SetDirection(Options[FMath::RandRange(0, Options.Num() - 1)]);
    return true;
}


bool UPacGhostDecisionComponent::ChooseRandomDirection()
{
    check(Maze);
    check(MovementComponent);

    FPacGridPosition const CurrentPosition = MovementComponent->GetCurrentGridPosition();

    UE_LOG(LogPacMan,
           Verbose,
           TEXT("PAC: Ghost ChooseDirection at (%d,%d)"),
           CurrentPosition.X,
           CurrentPosition.Y);

    TArray<EPacDirection> ValidDirections;

    EPacDirection const Directions[] = {EPacDirection::Up,
                                        EPacDirection::Down,
                                        EPacDirection::Left,
                                        EPacDirection::Right};

    EPacDirection const OppositeTravel = GetOppositeDirection(
        MovementComponent->GetTravelDirection());
    for (EPacDirection const Direction : Directions)
    {
        if (!IsDirectionWalkable(CurrentPosition, Direction))
        {
            continue;
        }
        // No reversing unless the ghost is allowed to (mode switch).
        if (Direction == OppositeTravel && !bAllowReverseOnNextDecision)
        {
            continue;
        }

        ValidDirections.Add(Direction);
    }
    // Dead end: reversing is the only option.
    if (ValidDirections.IsEmpty() && OppositeTravel != EPacDirection::None &&
        IsDirectionWalkable(CurrentPosition, OppositeTravel))
    {
        ValidDirections.Add(OppositeTravel);
    }

    if (ValidDirections.IsEmpty())
    {
        return false;
    }

    int32 const RandomIndex = FMath::RandRange(0, ValidDirections.Num() - 1);
    EPacDirection const ChosenDirection = ValidDirections[RandomIndex];

    UE_LOG(LogPacMan,
           Verbose,
           TEXT("PAC: Ghost chose direction %d from %d valid directions."),
           static_cast<uint8>(ChosenDirection),
           ValidDirections.Num());

    MovementComponent->SetDirection(ChosenDirection);
    return true;
}
