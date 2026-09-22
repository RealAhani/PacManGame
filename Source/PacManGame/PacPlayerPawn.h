// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PacGridTypes.h"
#include "PacPlayerPawn.generated.h"
// clang-format on

class USphereComponent;
class UPacGridMovementComponent;
class APacMaze;
UCLASS()
class PACMANGAME_API APacPlayerPawn : public APawn {
  GENERATED_BODY()

public:
  // Sets default values for this pawn's properties
  APacPlayerPawn();
  void SetDesiredDirection(EPacDirection Direction);
  void InitializeMaze(APacMaze *Maze);
  bool IsMovementBlocked() const;

private:
  UPROPERTY(VisibleAnywhere, Category = "Components")
  TObjectPtr<USphereComponent> CollisionComponent;

  UPROPERTY(VisibleAnywhere, Category = "Components")
  TObjectPtr<UPacGridMovementComponent> GridMovementComponent;
};
