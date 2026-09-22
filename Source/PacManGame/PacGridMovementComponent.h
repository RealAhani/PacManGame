// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "PacGridTypes.h"
#include "PacGridMovementComponent.generated.h"
// clang-format on

class APacMaze;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGridPositionChanged,
                                    const FPacGridPosition &);

UCLASS(ClassGroup = (PacMan), meta = (BlueprintSpawnableComponent))
class PACMANGAME_API UPacGridMovementComponent : public UPawnMovementComponent {
  GENERATED_BODY()

public:
  // -------------------- functions --------------------
  UPacGridMovementComponent();

  void SetDesiredDirection(EPacDirection Direction);

  FPacGridPosition GetCurrentGridPosition() const;

  void InitializeMaze(APacMaze *InMaze);

  bool IsMovementBlocked() const { return bMovementBlocked; }

  // -------------------- properties --------------------
  FOnGridPositionChanged OnGridPositionChanged;

protected:
  virtual void BeginPlay() override;

  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

private:
  // -------------------- functions --------------------
  bool TryStartMovement();
  void MoveTowardTarget(float DeltaTime);

  // -------------------- properties --------------------
  UPROPERTY(EditInstanceOnly, Category = "Grid Movement")
  TObjectPtr<APacMaze> Maze;

  UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
  FPacGridPosition CurrentGridPosition;

  UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
  EPacDirection CurrentDirection = EPacDirection::None;

  UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
  TArray<EPacDirection> InputQueue;

  UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
  FPacGridPosition TargetGridPosition;

  UPROPERTY(EditDefaultsOnly, Category = "Grid Movement",
            meta = (ClampMin = "0.0"))
  float MovementSpeed = 300.0f;

  bool bMovementBlocked = false;
};
