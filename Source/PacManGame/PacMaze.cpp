// Fill out your copyright notice in the Description page of Project Settings.

#include "PacMaze.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetStringLibrary.h"
#include "PacMazeDataAsset.h"

// Sets default values
APacMaze::APacMaze() {
  // Set this actor to call Tick() every frame.  You can turn this off to
  // improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void APacMaze::BeginPlay() {
  Super::BeginPlay();

  check(MazeData);

  GridSize = MazeData->GridSize;
  if (!ValidateMazeData()) {
    return;
  }
  BuildRuntimeGrid();
  DrawDebugMaze();
}

bool APacMaze::ValidateMazeData() const {
  if (GridSize.X <= 0 || GridSize.Y <= 0) {
    UE_LOG(LogTemp, Error, TEXT("Maze '%s' has an invalid grid size: %d x %d."),
           *MazeData->GetName(), GridSize.X, GridSize.Y);

    return false;
  }

  TArray<FString> Rows;
  if (!GetLayoutRows(Rows)) {
    UE_LOG(LogTemp, Error, TEXT("Maze '%s' has an empty layout."),
           *MazeData->GetName());

    return false;
  }

  if (Rows.Num() != GridSize.Y) {
    UE_LOG(LogTemp, Error, TEXT("Maze '%s' has %d rows, but expected %d."),
           *MazeData->GetName(), Rows.Num(), GridSize.Y);

    return false;
  }

  for (int32 Y = 0; Y < GridSize.Y; ++Y) {
    if (Rows[Y].Len() != GridSize.X) {
      UE_LOG(LogTemp, Error,
             TEXT("Maze '%s' row %d has %d columns, but expected %d."),
             *MazeData->GetName(), Y, Rows[Y].Len(), GridSize.X);

      return false;
    }

    for (int32 X = 0; X < GridSize.X; ++X) {
      const TCHAR Character = Rows[Y][X];
      if (Character != TEXT('#') && Character != TEXT('.') &&
          Character != TEXT('-')) {
        UE_LOG(LogTemp, Error,
               TEXT("Maze '%s' contains invalid character '%c' at (%d, %d)."),
               *MazeData->GetName(), Character, X, Y);

        return false;
      }
    }
  }
  const FPacGridPosition PlayerStart = MazeData->PlayerStart;

  if (!IsValidGridPosition(PlayerStart)) {
    UE_LOG(LogTemp, Error,
           TEXT("Maze '%s' has an invalid PlayerStart at (%d, %d)."),
           *MazeData->GetName(), PlayerStart.X, PlayerStart.Y);

    return false;
  }

  if (ParseTileCharacter(Rows[PlayerStart.Y][PlayerStart.X]) ==
      EPacTileType::Wall) {
    UE_LOG(
        LogTemp, Error,
        TEXT("Maze '%s' has PlayerStart at (%d, %d), but that cell is a wall."),
        *MazeData->GetName(), PlayerStart.X, PlayerStart.Y);

    return false;
  }
  return true;
}

void APacMaze::BuildRuntimeGrid() {
  TArray<FString> Rows;

  check(GetLayoutRows(Rows));

  const int32 TileCount = GridSize.X * GridSize.Y;

  Tiles.Init(EPacTileType::Empty, TileCount);

  for (int32 Y = 0; Y < GridSize.Y; ++Y) {
    for (int32 X = 0; X < GridSize.X; ++X) {
      const TCHAR Character = Rows[Y][X];
      const int32 Index = Y * GridSize.X + X;
      Tiles[Index] = ParseTileCharacter(Character);
    }
  }
}

bool APacMaze::GetLayoutRows(TArray<FString> &OutRows) const {
  OutRows.Reset();

  check(MazeData);

  FString NormalizedLayout = MazeData->Layout;

  NormalizedLayout.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
  NormalizedLayout.ReplaceInline(TEXT("\r"), TEXT("\n"));

  NormalizedLayout.ParseIntoArray(OutRows, TEXT("\n"), false);

  return OutRows.Num() > 0;
}

EPacTileType APacMaze::ParseTileCharacter(const TCHAR Character) const {
  switch (Character) {
  case TEXT('#'):
    return EPacTileType::Wall;

  case TEXT('.'):
    return EPacTileType::Pellet;

  // case TEXT(' '):
  //   return EPacTileType::Empty;
  case TEXT('-'):
    return EPacTileType::Empty;
  default:
    checkNoEntry();
    return EPacTileType::Empty;
  }
}

bool APacMaze::IsValidGridPosition(const FPacGridPosition &Position) const {
  return Position.X >= 0 && Position.X < GridSize.X && Position.Y >= 0 &&
         Position.Y < GridSize.Y;
}

int32 APacMaze::GetTileIndex(const FPacGridPosition &Position) const {
  check(IsValidGridPosition(Position));

  return Position.Y * GridSize.X + Position.X;
}

EPacTileType APacMaze::GetTile(const FPacGridPosition &Position) const {
  check(IsValidGridPosition(Position));

  return Tiles[GetTileIndex(Position)];
}

void APacMaze::SetTile(const FPacGridPosition &Position,
                       EPacTileType TileType) {
  check(IsValidGridPosition(Position));

  Tiles[GetTileIndex(Position)] = TileType;
}

bool APacMaze::IsWalkable(const FPacGridPosition &Position) const {
  if (!IsValidGridPosition(Position)) {
    return false;
  }

  return GetTile(Position) != EPacTileType::Wall;
}

FVector APacMaze::GridToWorld(const FPacGridPosition &Position) const {
  check(IsValidGridPosition(Position));

  const FVector LocalPosition(
      (static_cast<float>(Position.X) + 0.5f) * TileSize,
      (static_cast<float>(Position.Y) + 0.5f) * TileSize, 0.0f);

  return GetActorTransform().TransformPosition(LocalPosition);
}

FPacGridPosition APacMaze::WorldToGrid(const FVector &WorldPosition) const {
  const FVector LocalPosition =
      GetActorTransform().InverseTransformPosition(WorldPosition);

  const int32 X = FMath::FloorToInt(LocalPosition.X / TileSize);
  const int32 Y = FMath::FloorToInt(LocalPosition.Y / TileSize);

  return FPacGridPosition(X, Y);
}

FIntPoint APacMaze::GetGridSize() const { return GridSize; }

float APacMaze::GetTileSize() const { return TileSize; }

bool APacMaze::TryGetNeighbor(const FPacGridPosition &Position,
                              const EPacDirection Direction,
                              FPacGridPosition &OutNeighbor) const {
  const FPacGridPosition Neighbor = GetNeighbor(Position, Direction);

  if (!IsValidGridPosition(Neighbor)) {
    return false;
  }

  OutNeighbor = Neighbor;
  return true;
}

bool APacMaze::TryGetTile(const FPacGridPosition &Position,
                          EPacTileType &OutTile) const {
  if (!IsValidGridPosition(Position)) {
    return false;
  }

  OutTile = Tiles[GetTileIndex(Position)];
  return true;
}

void APacMaze::DrawDebugMaze() const {
  for (int32 Y = 0; Y < GridSize.Y; ++Y) {
    for (int32 X = 0; X < GridSize.X; ++X) {
      const FPacGridPosition Position(X, Y);
      const EPacTileType Tile = GetTile(Position);

      const FVector WorldPosition = GridToWorld(Position);

      const FVector BoxExtent(TileSize * 0.45f, TileSize * 0.45f, 10.0f);

      if (Tile == EPacTileType::Wall) {
        DrawDebugBox(GetWorld(), WorldPosition, BoxExtent, FColor::Blue, true,
                     -1.0f, 0, 2.0f);
      } else if (Tile == EPacTileType::Pellet) {
        DrawDebugPoint(GetWorld(), WorldPosition, 10.0f, FColor::Yellow, true,
                       -1.0f);
      }
    }
  }
  const FPacGridPosition PlayerStart = GetPlayerStart();

  DrawDebugSphere(GetWorld(), GridToWorld(PlayerStart), TileSize * 0.2f, 16,
                  FColor::Green, true, -1.0f, 0, 2.0f);
}

FPacGridPosition APacMaze::GetPlayerStart() const {
  check(MazeData);

  return MazeData->PlayerStart;
}