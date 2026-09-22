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
class PACMANGAME_API APacMaze : public AActor {
  GENERATED_BODY()

public:
  // Sets default values for this actor's properties
  APacMaze();

  bool IsValidGridPosition(const FPacGridPosition &Position) const;

  EPacTileType GetTile(const FPacGridPosition &Position) const;
  void SetTile(const FPacGridPosition &Position, EPacTileType TileType);

  bool IsWalkable(const FPacGridPosition &Position) const;

  FVector GridToWorld(const FPacGridPosition &Position) const;
  FPacGridPosition WorldToGrid(const FVector &WorldPosition) const;

  FIntPoint GetGridSize() const;
  float GetTileSize() const;

  bool TryGetNeighbor(const FPacGridPosition &Position, EPacDirection Direction,
                      FPacGridPosition &OutNeighbor) const;

  bool TryGetTile(const FPacGridPosition &Position,
                  EPacTileType &OutTile) const;

  // DEBUG_DRAW
  void DrawDebugMaze() const;

  FPacGridPosition GetPlayerStart() const;

  void InitializeRuntime();

  UCameraComponent *GetCameraComponent() const;

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;
  virtual void PostInitializeComponents() override;

private:
  // -------------------- functions --------------------
  int32 GetTileIndex(const FPacGridPosition &Position) const;
  bool ValidateMazeData() const;
  void BuildRuntimeGrid();
  bool GetLayoutRows(TArray<FString> &OutRows) const;
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
};
