// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacPlayerPawn.h"
#include "PacGridMovementComponent.h"
#include "Components/SphereComponent.h"

#include "PacMaze.h"
// clang-format on

APacPlayerPawn::APacPlayerPawn() {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.bStartWithTickEnabled = false;

  CollisionComponent =
      CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));

  SetRootComponent(CollisionComponent);

  CollisionComponent->SetSphereRadius(40.0f);

  GridMovementComponent = CreateDefaultSubobject<UPacGridMovementComponent>(
      TEXT("GridMovementComponent"));

  GridMovementComponent->SetUpdatedComponent(CollisionComponent);
}

void APacPlayerPawn::SetDesiredDirection(const EPacDirection Direction) {
  check(GridMovementComponent);

  GridMovementComponent->SetDesiredDirection(Direction);
}

void APacPlayerPawn::InitializeMaze(APacMaze *Maze) {
  check(GridMovementComponent);
  GridMovementComponent->InitializeMaze(Maze);
}

bool APacPlayerPawn::IsMovementBlocked() const {
  check(GridMovementComponent);

  return GridMovementComponent->IsMovementBlocked();
}

UPacGridMovementComponent *APacPlayerPawn::GetGridMovementComponent() const {
  return GridMovementComponent;
}