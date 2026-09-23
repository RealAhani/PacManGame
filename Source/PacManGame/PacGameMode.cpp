// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacGameMode.h"
#include "PacMaze.h"
#include "PacPelletManager.h"
#include "PacPlayerState.h"

#include "Kismet/GameplayStatics.h"
// clang-format on

APacGameMode::APacGameMode() {
  PlayerStateClass = APacPlayerState::StaticClass();
}

void APacGameMode::BeginPlay() {
  Super::BeginPlay();

  Maze = Cast<APacMaze>(
      UGameplayStatics::GetActorOfClass(GetWorld(), APacMaze::StaticClass()));

  check(Maze);

  PelletManager = Cast<APacPelletManager>(UGameplayStatics::GetActorOfClass(
      GetWorld(), APacPelletManager::StaticClass()));

  check(PelletManager);

  PelletManager->Initialize(Maze);

  UE_LOG(LogTemp, Warning, TEXT("PAC: GameMode found maze '%s'."),
         *Maze->GetName());
  PelletManager->OnAllPelletsConsumed.AddUObject(
      this, &APacGameMode::HandleAllPelletsConsumed);
}

APacMaze *APacGameMode::GetMaze() { return Maze; }

APacPelletManager *APacGameMode::GetPelletManager() const {
  return PelletManager;
}

void APacGameMode::HandleAllPelletsConsumed() {
  UE_LOG(LogTemp, Warning, TEXT("PAC: All pellets consumed. Level complete."));
}