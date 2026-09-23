// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacPelletManager.h"

#include "PacMaze.h"
#include "PacPellet.h"
#include "PacGridMovementComponent.h"
// clang-format on

// Sets default values
APacPelletManager::APacPelletManager() {
  PrimaryActorTick.bCanEverTick = false;
}

void APacPelletManager::Initialize(APacMaze *InMaze) {
  check(InMaze);

  Maze = InMaze;

  SpawnPellets();
}

void APacPelletManager::SpawnPellets() {
  check(Maze);

  if (PelletClass == nullptr) {
    UE_LOG(LogTemp, Error,
           TEXT("PAC: PelletManager has no PelletClass assigned."));
    return;
  }

  const FIntPoint GridSize = Maze->GetGridSize();

  for (int32 Y = 0; Y < GridSize.Y; ++Y) {
    for (int32 X = 0; X < GridSize.X; ++X) {
      const FPacGridPosition Position(X, Y);

      const EPacTileType TileType = Maze->GetTile(Position);

      TSubclassOf<APacPellet> PelletToSpawn;

      switch (TileType) {
      case EPacTileType::Pellet:
        PelletToSpawn = PelletClass;
        break;

      case EPacTileType::PowerPellet:
        PelletToSpawn = PowerPelletClass;
        break;

      default:
        continue;
      }

      if (PelletToSpawn == nullptr) {
        UE_LOG(LogTemp, Error,
               TEXT("PAC: No pellet class assigned for tile at (%d,%d)."), X,
               Y);

        continue;
      }

      const FVector WorldPosition = Maze->GridToWorld(Position);

      FActorSpawnParameters SpawnParameters;
      SpawnParameters.Owner = this;

      APacPellet *Pellet = GetWorld()->SpawnActor<APacPellet>(
          PelletToSpawn, WorldPosition, FRotator::ZeroRotator, SpawnParameters);

      if (Pellet == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("PAC: Failed to spawn pellet at (%d,%d)."),
               X, Y);

        continue;
      }
      Pellet->Initialize(Position);

      Pellets.Add(Position, Pellet);
    }
  }

  UE_LOG(LogTemp, Warning, TEXT("PAC: Spawned %d pellets."), Pellets.Num());
}

void APacPelletManager::HandlePlayerGridPositionChanged(
    const FPacGridPosition &Position) {
  TObjectPtr<APacPellet> *FoundPellet = Pellets.Find(Position);

  if (FoundPellet == nullptr) {
    return;
  }

  APacPellet *Pellet = *FoundPellet;
  if (!IsValid(Pellet)) {
    UE_LOG(LogTemp, Warning,
           TEXT("PAC: Pellet for delettion is not valid "
                ":HandlePlayerGridPositionChanged"));
    Pellets.Remove(Position);
    return;
  }

  const int32 ScoreValue = Pellet->GetScoreValue();

  Pellet->Destroy();

  Pellets.Remove(Position);

  OnPelletConsumed.Broadcast(ScoreValue);

  UE_LOG(LogTemp, Warning,
         TEXT("PAC: Pellet consumed at (%d,%d). Value=%d, Remaining=%d"),
         Position.X, Position.Y, ScoreValue, Pellets.Num());
}

void APacPelletManager::BindToPlayerMovement(
    UPacGridMovementComponent *MovementComponent) {
  check(MovementComponent);

  MovementComponent->OnGridPositionChanged.AddUObject(
      this, &APacPelletManager::HandlePlayerGridPositionChanged);
}
