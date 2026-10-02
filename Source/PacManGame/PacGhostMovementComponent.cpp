// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacGhostMovementComponent.h"
#include "PacMaze.h"
// clang-format on

UPacGhostMovementComponent::UPacGhostMovementComponent()
{
    PrimaryComponentTick.bCanEverTick          = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    bAutoActivate = true;
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

    if (CurrentGridPosition == TargetGridPosition)
    {
        TryStartMovement();
    }

    if (CurrentGridPosition != TargetGridPosition)
    {
        MoveTowardTarget(DeltaTime);
    }
}

void UPacGhostMovementComponent::SetDirection(EPacDirection const Direction)
{
    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: Ghost SetDirection = %d"),
           static_cast<uint8>(Direction));

    CurrentDirection = Direction;
}

FPacGridPosition UPacGhostMovementComponent::GetCurrentGridPosition() const
{
    return CurrentGridPosition;
}

void UPacGhostMovementComponent::InitializeMaze(APacMaze* InMaze,
                                                FPacGridPosition const & StartPosition)
{
    check(InMaze);

    Maze = InMaze;

    check(Maze->IsWalkable(StartPosition));

    CurrentGridPosition = StartPosition;
    TargetGridPosition  = StartPosition;
    CurrentDirection    = EPacDirection::None;

    APawn* OwnerPawn = GetPawnOwner();
    check(OwnerPawn);

    OwnerPawn->SetActorLocation(Maze->GridToWorld(CurrentGridPosition));
}

bool UPacGhostMovementComponent::TryStartMovement()
{
    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: Ghost TryStartMovement. Position=(%d,%d), Direction=%d"),
           CurrentGridPosition.X,
           CurrentGridPosition.Y,
           static_cast<uint8>(CurrentDirection));

    if (CurrentDirection == EPacDirection::None)
    {
        return false;
    }

    FPacGridPosition NextPosition;

    if (!Maze->TryGetNeighbor(CurrentGridPosition, CurrentDirection, NextPosition))
    {
        return false;
    }

    if (!Maze->IsWalkable(NextPosition))
    {
        return false;
    }

    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: Ghost starting movement to (%d,%d)."),
           NextPosition.X,
           NextPosition.Y);

    TargetGridPosition = NextPosition;

    return true;
}

void UPacGhostMovementComponent::MoveTowardTarget(float const DeltaTime)
{
    check(Maze);
    check(UpdatedComponent);

    FVector const TargetWorldPosition = Maze->GridToWorld(TargetGridPosition);

    FVector const CurrentWorldPosition = UpdatedComponent->GetComponentLocation();

    FVector const ToTarget = TargetWorldPosition - CurrentWorldPosition;

    float const DistanceToTarget = ToTarget.Size();

    if (DistanceToTarget <= KINDA_SMALL_NUMBER)
    {
        UpdatedComponent->SetWorldLocation(TargetWorldPosition);

        CurrentGridPosition = TargetGridPosition;
        OnGridPositionChanged.Broadcast(CurrentGridPosition);

        return;
    }

    float const DistanceThisFrame = MovementSpeed * DeltaTime;

    if (DistanceThisFrame >= DistanceToTarget)
    {
        UpdatedComponent->SetWorldLocation(TargetWorldPosition);

        CurrentGridPosition = TargetGridPosition;
        OnGridPositionChanged.Broadcast(CurrentGridPosition);

        return;
    }

    FVector const MovementDelta = ToTarget.GetSafeNormal() * DistanceThisFrame;

    FHitResult Hit;

    SafeMoveUpdatedComponent(MovementDelta,
                             UpdatedComponent->GetComponentQuat(),
                             false,
                             Hit,
                             ETeleportType::None);
}