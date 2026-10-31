// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PacGridTypes.h"
#include "PacMaze.generated.h"
// clang-format on

class UPacMazeDataAsset;
class USceneComponent;
class UCameraComponent;

UCLASS()
class PACMANGAME_API APacMaze : public AActor
{
    GENERATED_BODY()

public:

    // Sets default values for this actor's properties
    APacMaze();
    // -------------------- functions --------------------
    bool         IsValidGridPosition(FPacGridPosition const & Position) const;
    EPacTileType GetTile(FPacGridPosition const & Position) const;
    void    SetTile(FPacGridPosition const & Position, EPacTileType TileType);
    bool    IsWalkable(FPacGridPosition const & Position) const;
    FVector GridToWorld(FPacGridPosition const & Position) const;
    FPacGridPosition WorldToGrid(FVector const & WorldPosition) const;
    FIntPoint        GetGridSize() const;
    float            GetTileSize() const;
    bool             TryGetNeighbor(FPacGridPosition const & Position,
                                    EPacDirection            Direction,
                                    FPacGridPosition &       OutNeighbor) const;
    bool TryGetTile(FPacGridPosition const & Position, EPacTileType & OutTile) const;
    void                             DrawDebugMaze() const;  // DEBUG_DRAW
    FPacGridPosition                 GetPlayerStart() const;
    TArray<FPacGridPosition> const & GetGhostStarts() const;
    void                             InitializeRuntime();
    UCameraComponent*                GetCameraComponent() const;


protected:

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;

private:

    // -------------------- functions --------------------
    int32        GetTileIndex(FPacGridPosition const & Position) const;
    bool         ValidateMazeData() const;
    void         BuildRuntimeGrid();
    bool         GetLayoutRows(TArray<FString> & OutRows) const;
    EPacTileType ParseTileCharacter(TCHAR Character) const;

    // -------------------- properties --------------------
    bool bRuntimeInitialized = false;

    UPROPERTY(EditDefaultsOnly, Category = "Maze")
    TObjectPtr<UPacMazeDataAsset> MazeData;

    UPROPERTY(VisibleInstanceOnly, Category = "Maze|Grid")
    FIntPoint GridSize = FIntPoint::ZeroValue;

    //   TileSize is the width/height of one logical cell in Unreal units.
    UPROPERTY(EditDefaultsOnly, Category = "Maze|Grid", meta = (ClampMin = "1.0"))
    float TileSize = 100.0f;

    UPROPERTY(VisibleInstanceOnly, Category = "Maze|Grid")
    TArray<EPacTileType> Tiles;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USceneComponent> BaseComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UCameraComponent> CameraComponent;

public:

    // -------------------- properties --------------------
    FVector const WallExtent = FVector(50.0, 50.0, 10.0);
};
