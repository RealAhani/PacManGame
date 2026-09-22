// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacPlayerController.h"
#include "PacPlayerPawn.h"
#include "PacGameMode.h"
#include "PacMaze.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
// clang-format on

void APacPlayerController::BeginPlay() {
  Super::BeginPlay();

  if (!IsLocalController()) {
    return;
  }

  ULocalPlayer *LocalPlayer = GetLocalPlayer();

  if (LocalPlayer == nullptr) {
    return;
  }

  UEnhancedInputLocalPlayerSubsystem *InputSubsystem =
      LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

  if (InputSubsystem == nullptr) {
    return;
  }

  if (PlayerMappingContext == nullptr) {
    return;
  }

  InputSubsystem->AddMappingContext(PlayerMappingContext, 0);

  APacGameMode *GameMode = GetWorld()->GetAuthGameMode<APacGameMode>();
  if (GameMode == nullptr) {
    return;
  }
  APacMaze *Maze = GameMode->GetMaze();
  if (Maze == nullptr) {
    return;
  }
  // target camera of player is in maze
  SetViewTarget(Maze);

  APacPlayerPawn *PlayerPawn = Cast<APacPlayerPawn>(GetPawn());
  if (PlayerPawn == nullptr) {
    return;
  }
  PlayerPawn->InitializeMaze(Maze);
}

void APacPlayerController::SetupInputComponent() {
  Super::SetupInputComponent();

  UEnhancedInputComponent *EnhancedInputComponent =
      Cast<UEnhancedInputComponent>(InputComponent);

  if (EnhancedInputComponent == nullptr) {
    return;
  }

  if (MoveAction == nullptr) {
    return;
  }

  EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this,
                                     &APacPlayerController::HandleMove);
}

void APacPlayerController::HandleMove(const FInputActionValue &Value) {
  const FVector2D MovementInput = Value.Get<FVector2D>();

  UE_LOG(LogTemp, Log, TEXT("Move Input: X=%f Y=%f"), MovementInput.X,
         MovementInput.Y);
  MoveInput = MovementInput;

  if (MoveInput.IsNearlyZero()) {
    return;
  }

  EPacDirection Direction = EPacDirection::None;

  if (FMath::Abs(MoveInput.X) > FMath::Abs(MoveInput.Y)) {
    Direction = MoveInput.X > 0.0f ? EPacDirection::Left : EPacDirection::Right;
  } else {
    Direction = MoveInput.Y > 0.0f ? EPacDirection::Up : EPacDirection::Down;
  }
  if (Direction == LastSubmittedDirection) {
    return;
  }

  LastSubmittedDirection = Direction;

  APacPlayerPawn *PlayerPawn = Cast<APacPlayerPawn>(GetPawn());

  if (PlayerPawn == nullptr) {
    return;
  }

  PlayerPawn->SetDesiredDirection(Direction);
}
