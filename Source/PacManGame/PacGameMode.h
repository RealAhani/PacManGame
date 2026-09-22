// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PacGameMode.generated.h"
// clang-format on

class APacMaze;
class APacPelletManager;
class APacPlayerState;

UCLASS()
class PACMANGAME_API APacGameMode : public AGameModeBase {
  GENERATED_BODY()
public:
  APacGameMode();
  virtual void BeginPlay() override;
  APacMaze *GetMaze();
  APacPelletManager *GetPelletManager() const;

private:
  UPROPERTY()
  TObjectPtr<APacMaze> Maze;
  UPROPERTY()
  TObjectPtr<APacPelletManager> PelletManager;
};
