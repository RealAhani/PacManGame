// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacGameMode.h"
#include "PacMaze.h"
#include "Kismet/GameplayStatics.h"
// clang-format on

APacGameMode::APacGameMode() {}

void APacGameMode::BeginPlay() {
  Super::BeginPlay();

  Maze = Cast<APacMaze>(
      UGameplayStatics::GetActorOfClass(GetWorld(), APacMaze::StaticClass()));

  check(Maze);

  UE_LOG(LogTemp, Warning, TEXT("PAC: GameMode found maze '%s'."),
         *Maze->GetName());
}

APacMaze *APacGameMode::GetMaze() { return Maze; }