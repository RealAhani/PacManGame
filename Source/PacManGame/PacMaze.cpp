// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacMaze.h"

#include "DrawDebugHelpers.h"
#include "Kismet/KismetStringLibrary.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"

#include "PacMazeDataAsset.h"
// clang-format on

// Sets default values
APacMaze::APacMaze()
{
    PrimaryActorTick.bCanEverTick = false;
    BaseComponent                 = CreateDefaultSubobject<USceneComponent>(
        TEXT("RootComponent"));

    SetRootComponent(BaseComponent);

    CameraComponent = CreateDefaultSubobject<UCameraComponent>(
        TEXT("CameraComponent"));

    CameraComponent->SetupAttachment(BaseComponent);
    // CameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;

    // CameraComponent->OrthoWidth = 3200.0f;

    // CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 2000.0f));

    // CameraComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
}

// Called when the game starts or when spawned
void APacMaze::BeginPlay()
{
    Super::BeginPlay();

    InitializeRuntime();
    DrawDebugMaze();
}

void APacMaze::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (MazeData == nullptr)
    {
        return;
    }

    InitializeRuntime();
}

void APacMaze::InitializeRuntime()
{
    if (bRuntimeInitialized)
    {
        return;
    }

    if (MazeData == nullptr)
    {
        return;
    }

    GridSize = MazeData->GridSize;

    if (!ValidateMazeData())
    {
        return;
    }

    BuildRuntimeGrid();

    bRuntimeInitialized = true;

    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: Maze initialized. Size=(%d,%d), Data=%s"),
           GridSize.X,
           GridSize.Y,
           *GetNameSafe(MazeData));
}

bool APacMaze::ValidateMazeData() const
{
    if (GridSize.X <= 0 || GridSize.Y <= 0)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("Maze '%s' has an invalid grid size: %d x %d."),
               *MazeData->GetName(),
               GridSize.X,
               GridSize.Y);

        return false;
    }

    TArray<FString> Rows;
    if (!GetLayoutRows(Rows))
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("Maze '%s' has an empty layout."),
               *MazeData->GetName());

        return false;
    }

    if (Rows.Num() != GridSize.Y)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("Maze '%s' has %d rows, but expected %d."),
               *MazeData->GetName(),
               Rows.Num(),
               GridSize.Y);

        return false;
    }

    for (int32 Y = 0; Y < GridSize.Y; ++Y)
    {
        if (Rows[Y].Len() != GridSize.X)
        {
            UE_LOG(LogTemp,
                   Error,
                   TEXT("Maze '%s' row %d has %d columns, but expected %d."),
                   *MazeData->GetName(),
                   Y,
                   Rows[Y].Len(),
                   GridSize.X);

            return false;
        }

        for (int32 X = 0; X < GridSize.X; ++X)
        {
            const TCHAR Character = Rows[Y][X];
            if (Character != TEXT('#') && Character != TEXT('.') &&
                Character != TEXT('-') && Character != TEXT('o'))
            {
                UE_LOG(LogTemp,
                       Error,
                       TEXT("Maze '%s' contains invalid character '%c' at (%d, "
                            "%d)."),
                       *MazeData->GetName(),
                       Character,
                       X,
                       Y);

                return false;
            }
        }
    }
    FPacGridPosition const PlayerStart = MazeData->PlayerStart;

    if (!IsValidGridPosition(PlayerStart))
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("Maze '%s' has an invalid PlayerStart at (%d, %d)."),
               *MazeData->GetName(),
               PlayerStart.X,
               PlayerStart.Y);

        return false;
    }

    for (int32 Index = 0; Index < MazeData->GhostStarts.Num(); ++Index)
    {
        FPacGridPosition const GhostStart = MazeData->GhostStarts[Index];

        if (!IsValidGridPosition(GhostStart))
        {
            UE_LOG(LogTemp,
                   Error,
                   TEXT("Maze '%s' has invalid GhostStart[%d] at (%d, %d)."),
                   *MazeData->GetName(),
                   Index,
                   GhostStart.X,
                   GhostStart.Y);

            return false;
        }

        if (ParseTileCharacter(Rows[GhostStart.Y][GhostStart.X]) ==
            EPacTileType::Wall)
        {
            UE_LOG(LogTemp,
                   Error,
                   TEXT("Maze '%s' has GhostStart[%d] at (%d, %d), "
                        "but that cell is a wall."),
                   *MazeData->GetName(),
                   Index,
                   GhostStart.X,
                   GhostStart.Y);

            return false;
        }
    }

    if (ParseTileCharacter(Rows[PlayerStart.Y][PlayerStart.X]) == EPacTileType::Wall)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("Maze '%s' has PlayerStart at (%d, %d), but that cell is a "
                    "wall."),
               *MazeData->GetName(),
               PlayerStart.X,
               PlayerStart.Y);

        return false;
    }
    return true;
}

void APacMaze::BuildRuntimeGrid()
{
    TArray<FString> Rows;

    check(GetLayoutRows(Rows));

    int32 const TileCount = GridSize.X * GridSize.Y;

    Tiles.Init(EPacTileType::Empty, TileCount);

    for (int32 Y = 0; Y < GridSize.Y; ++Y)
    {
        for (int32 X = 0; X < GridSize.X; ++X)
        {
            const TCHAR Character = Rows[Y][X];
            int32 const Index     = Y * GridSize.X + X;
            Tiles[Index]          = ParseTileCharacter(Character);
        }
    }
}

bool APacMaze::GetLayoutRows(TArray<FString> & OutRows) const
{
    OutRows.Reset();

    check(MazeData);

    FString NormalizedLayout = MazeData->Layout;

    NormalizedLayout.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
    NormalizedLayout.ReplaceInline(TEXT("\r"), TEXT("\n"));

    NormalizedLayout.ParseIntoArray(OutRows, TEXT("\n"), false);

    return OutRows.Num() > 0;
}

EPacTileType APacMaze::ParseTileCharacter(const TCHAR Character) const
{
    switch (Character)
    {
        case TEXT('#'):
            return EPacTileType::Wall;

        case TEXT('.'):
            return EPacTileType::Pellet;

        case TEXT('-'):
            return EPacTileType::Empty;

        case TEXT('o'):
            return EPacTileType::PowerPellet;

        default:
            checkNoEntry();
            return EPacTileType::Empty;
    }
}

bool APacMaze::IsValidGridPosition(FPacGridPosition const & Position) const
{
    return Position.X >= 0 && Position.X < GridSize.X && Position.Y >= 0 &&
           Position.Y < GridSize.Y;
}

int32 APacMaze::GetTileIndex(FPacGridPosition const & Position) const
{
    check(IsValidGridPosition(Position));

    return Position.Y * GridSize.X + Position.X;
}

EPacTileType APacMaze::GetTile(FPacGridPosition const & Position) const
{
    check(IsValidGridPosition(Position));

    return Tiles[GetTileIndex(Position)];
}

void APacMaze::SetTile(FPacGridPosition const & Position, EPacTileType TileType)
{
    check(IsValidGridPosition(Position));

    Tiles[GetTileIndex(Position)] = TileType;
}

bool APacMaze::IsWalkable(FPacGridPosition const & Position) const
{
    if (!IsValidGridPosition(Position))
    {
        return false;
    }

    return GetTile(Position) != EPacTileType::Wall;
}

FVector APacMaze::GridToWorld(FPacGridPosition const & Position) const
{
    check(IsValidGridPosition(Position));

    FVector const LocalPosition((static_cast<float>(Position.X) + 0.5f) * TileSize,
                                (static_cast<float>(Position.Y) + 0.5f) * TileSize,
                                0.0f);

    return GetActorTransform().TransformPosition(LocalPosition);
}

FPacGridPosition APacMaze::WorldToGrid(FVector const & WorldPosition) const
{
    FVector const LocalPosition = GetActorTransform().InverseTransformPosition(
        WorldPosition);

    int32 const X = FMath::FloorToInt(LocalPosition.X / TileSize);
    int32 const Y = FMath::FloorToInt(LocalPosition.Y / TileSize);

    return FPacGridPosition(X, Y);
}

FIntPoint APacMaze::GetGridSize() const
{
    return GridSize;
}

float APacMaze::GetTileSize() const
{
    return TileSize;
}

bool APacMaze::TryGetNeighbor(FPacGridPosition const & Position,
                              EPacDirection const      Direction,
                              FPacGridPosition &       OutNeighbor) const
{
    FPacGridPosition const Neighbor = GetNeighbor(Position, Direction);

    if (!IsValidGridPosition(Neighbor))
    {
        return false;
    }

    OutNeighbor = Neighbor;
    return true;
}

bool APacMaze::TryGetTile(FPacGridPosition const & Position,
                          EPacTileType &           OutTile) const
{
    if (!IsValidGridPosition(Position))
    {
        return false;
    }

    OutTile = Tiles[GetTileIndex(Position)];
    return true;
}

void APacMaze::DrawDebugMaze() const
{
    for (int32 Y = 0; Y < GridSize.Y; ++Y)
    {
        for (int32 X = 0; X < GridSize.X; ++X)
        {
            FPacGridPosition const Position(X, Y);
            EPacTileType const     Tile = GetTile(Position);

            FVector const WorldPosition = GridToWorld(Position);

            FVector const BoxExtent(TileSize * 0.45f, TileSize * 0.45f, 10.0f);

            if (Tile == EPacTileType::Wall)
            {
                DrawDebugBox(GetWorld(),
                             WorldPosition,
                             BoxExtent,
                             FColor::Blue,
                             true,
                             -1.0f,
                             0,
                             2.0f);
            }
            else if (Tile == EPacTileType::Pellet)
            {
                DrawDebugPoint(GetWorld(), WorldPosition, 10.0f, FColor::Yellow, true, -1.0f);
            }
        }
    }
    FPacGridPosition const PlayerStart = GetPlayerStart();

    DrawDebugSphere(GetWorld(),
                    GridToWorld(PlayerStart),
                    TileSize * 0.2f,
                    16,
                    FColor::Green,
                    true,
                    -1.0f,
                    0,
                    2.0f);
}

FPacGridPosition APacMaze::GetPlayerStart() const
{
    check(MazeData);

    return MazeData->PlayerStart;
}

TArray<FPacGridPosition> const & APacMaze::GetGhostStarts() const
{
    check(MazeData);

    return MazeData->GhostStarts;
}

UCameraComponent* APacMaze::GetCameraComponent() const
{
    return CameraComponent;
}
