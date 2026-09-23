#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PacGridTypes.h"
#include "PacPelletManager.generated.h"
// clang-format on

class APacMaze;
class APacPellet;
class UPacGridMovementComponent;

// -------------------- Delegate(Fptr,event) --------------------
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPelletConsumed, int32);
DECLARE_MULTICAST_DELEGATE(FOnAllPelletsConsumed);

UCLASS()
class PACMANGAME_API APacPelletManager : public AActor {
  GENERATED_BODY()

public:
  // -------------------- functions --------------------
  APacPelletManager();
  void Initialize(APacMaze *InMaze);
  void BindToPlayerMovement(UPacGridMovementComponent *MovementComponent);

  // -------------------- properties --------------------
  FOnPelletConsumed OnPelletConsumed;
  FOnAllPelletsConsumed OnAllPelletsConsumed;

private:
  // -------------------- functions --------------------
  void SpawnPellets();
  void HandlePlayerGridPositionChanged(const FPacGridPosition &Position);

  // -------------------- properties --------------------
  UPROPERTY(EditDefaultsOnly, Category = "Pellets")
  TSubclassOf<APacPellet> PelletClass;

  UPROPERTY(EditDefaultsOnly, Category = "Pellets")
  TSubclassOf<APacPellet> PowerPelletClass;

  UPROPERTY()
  TObjectPtr<APacMaze> Maze;

  UPROPERTY()
  TMap<FPacGridPosition, TObjectPtr<APacPellet>> Pellets;
};