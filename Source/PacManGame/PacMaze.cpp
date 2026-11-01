// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacMaze.h"

#include "DrawDebugHelpers.h"
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

    FString ParseError;

    if (!FPacGrid::TryParse(MazeData->Layout, MazeData->GridSize, Grid, ParseError))
    {
        UE_LOG(LogTemp, Error, TEXT("Maze '%s': %s"), *MazeData->GetName(), *ParseError);


        return;
    }

    GridSize = Grid.GetSize();

    if (!ValidateStartPositions())
    {
        // Leave the maze in a clean "not initialized" state.
        Grid     = FPacGrid();
        GridSize = FIntPoint::ZeroValue;
        return;
    }

    bRuntimeInitialized = true;

    UE_LOG(LogTemp,
           Warning,
           TEXT("PAC: Maze initialized. Size=(%d,%d), Data=%s"),
           GridSize.X,
           GridSize.Y,
           *GetNameSafe(MazeData));
}

bool APacMaze::ValidateStartPositions() const
{
    FPacGridPosition const PlayerStart = MazeData->PlayerStart;

    if (!Grid.IsValidPosition(PlayerStart))
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

        if (!Grid.IsValidPosition(GhostStart))
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

        if (!Grid.IsWalkable(GhostStart))
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

    if (!Grid.IsWalkable(PlayerStart))
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


bool APacMaze::IsValidGridPosition(FPacGridPosition const & Position) const
{
    return Grid.IsValidPosition(Position);
}

EPacTileType APacMaze::GetTile(FPacGridPosition const & Position) const
{
    return Grid.GetTile(Position);
}

void APacMaze::SetTile(FPacGridPosition const & Position, EPacTileType TileType)
{
    Grid.SetTile(Position, TileType);
}

bool APacMaze::IsWalkable(FPacGridPosition const & Position) const
{

    return Grid.IsWalkable(Position);
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
    return Grid.GetSize();
}

float APacMaze::GetTileSize() const
{
    return TileSize;
}

bool APacMaze::TryGetNeighbor(FPacGridPosition const & Position,
                              EPacDirection const      Direction,
                              FPacGridPosition &       OutNeighbor) const
{
    return Grid.TryGetNeighbor(Position, Direction, OutNeighbor);
}

bool APacMaze::TryGetTile(FPacGridPosition const & Position,
                          EPacTileType &           OutTile) const
{
    return Grid.TryGetTile(Position, OutTile);
}

void APacMaze::DrawDebugMaze() const
{
    // Nothing to draw if the layout failed to load (the error is already
    // logged); GetPlayerStart() below would also assert.
    if (!bRuntimeInitialized)
    {
        return;
    }
    for (int32 Y = 0; Y < GridSize.Y; ++Y)
    {
        for (int32 X = 0; X < GridSize.X; ++X)
        {
            FPacGridPosition const Position(X, Y);
            EPacTileType const     Tile = GetTile(Position);

            FVector const WorldPosition = GridToWorld(Position);

            FVector const BoxExtent = WallExtent;

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
