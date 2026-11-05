// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PacGridTypes.h"
#include "Grid/PacGrid.h"
#include "PacMaze.generated.h"
// clang-format on

class UPacMazeDataAsset;
class USceneComponent;
class UCameraComponent;

/**
 * The Unreal side of the maze: world positions, camera and debug drawing.
 * All tile data and grid rules live in FPacGrid (see Grid/PacGrid.h); the
 * functions below simply forward to it.
 */
UCLASS()
class PACMANGAME_API APacMaze : public AActor
{
    GENERATED_BODY()

public:

    // Sets default values for this actor's properties
    APacMaze();
    // -------------------- functions --------------------
    EPacTileType GetTile(FPacGridPosition const & Position) const;
    void SetTile(FPacGridPosition const & Position, EPacTileType TileType);
    bool IsValidGridPosition(FPacGridPosition const & Position) const;

    // Walkable = not a wall and not the ghost house; pass true to allow the house.
    bool    IsWalkable(FPacGridPosition const & Position,
                       bool                     bAllowGhostHouse = false) const;
    bool    IsGhostHouseTile(FPacGridPosition const & Position) const;
    FVector GridToWorld(FPacGridPosition const & Position) const;
    // Also accepts positions outside the grid (the tile just beyond a tunnel).
    FVector GridToWorldUnchecked(FPacGridPosition const & Position) const;
    FPacGridPosition WorldToGrid(FVector const & WorldPosition) const;
    FIntPoint        GetGridSize() const;
    float            GetTileSize() const;

    bool TryGetNeighbor(FPacGridPosition const & Position,
                        EPacDirection            Direction,
                        FPacGridPosition &       OutNeighbor) const;

    // Same as TryGetNeighbor, plus the tunnel (see FPacGrid).
    bool TryGetNeighborWrapping(FPacGridPosition const & Position,
                                EPacDirection            Direction,
                                FPacGridPosition &       OutNeighbor) const;
    bool TryGetTile(FPacGridPosition const & Position, EPacTileType & OutTile) const;

    void                             DrawDebugMaze() const;  // DEBUG_DRAW
    FPacGridPosition                 GetPlayerStart() const;
    TArray<FPacGridPosition> const & GetGhostStarts() const;
    FPacGridPosition                 GetGhostHouseExit() const;
    void                             InitializeRuntime();
    UCameraComponent*                GetCameraComponent() const;

    FPacGrid const & GetGrid() const
    {
        return Grid;
    }

    // True once the layout parsed and the start positions validated.
    bool IsRuntimeInitialized() const
    {
        return bRuntimeInitialized;
    }


protected:

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;

private:

    // -------------------- functions --------------------
    bool ValidateStartPositions() const;

    // -------------------- properties --------------------
    bool bRuntimeInitialized = false;
    // Plain C++ data, not a UPROPERTY: it holds no UObject pointers, so the
    // garbage collector has nothing to track.
    FPacGrid Grid;

    UPROPERTY(EditDefaultsOnly, Category = "Maze")
    TObjectPtr<UPacMazeDataAsset> MazeData;

    UPROPERTY(VisibleInstanceOnly, Category = "Maze|Grid")
    FIntPoint GridSize = FIntPoint::ZeroValue;

    //   TileSize is the width/height of one logical cell in Unreal units.
    UPROPERTY(EditDefaultsOnly, Category = "Maze|Grid", meta = (ClampMin = "1.0"))
    float TileSize = 100.0f;


    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USceneComponent> BaseComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UCameraComponent> CameraComponent;

public:

    // -------------------- properties --------------------
    FVector const WallExtent = FVector(50.0, 50.0, 10.0);
};
