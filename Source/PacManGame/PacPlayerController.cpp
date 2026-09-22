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

  if (MoveUpAction == nullptr || MoveDownAction == nullptr ||
      MoveLeftAction == nullptr || MoveRightAction == nullptr) {
    return;
  }

  EnhancedInputComponent->BindAction(MoveUpAction, ETriggerEvent::Triggered,
                                     this, &APacPlayerController::HandleMoveUp);

  EnhancedInputComponent->BindAction(MoveDownAction, ETriggerEvent::Triggered,
                                     this,
                                     &APacPlayerController::HandleMoveDown);

  EnhancedInputComponent->BindAction(MoveLeftAction, ETriggerEvent::Triggered,
                                     this,
                                     &APacPlayerController::HandleMoveLeft);

  EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Triggered,
                                     this,
                                     &APacPlayerController::HandleMoveRight);
}

void APacPlayerController::SubmitDirection(const EPacDirection Direction) {

  APacPlayerPawn *PlayerPawn = Cast<APacPlayerPawn>(GetPawn());

  if (PlayerPawn == nullptr) {
    return;
  }

  if (PlayerPawn->IsMovementBlocked()) {
    LastSubmittedDirection = EPacDirection::None;
  }

  if (Direction == LastSubmittedDirection) {
    return;
  }

  LastSubmittedDirection = Direction;

  PlayerPawn->SetDesiredDirection(Direction);

  UE_LOG(LogTemp, Log, TEXT("PAC: Direction submitted: %d"),
         static_cast<uint8>(Direction));
}

void APacPlayerController::HandleMoveUp(const FInputActionValue &Value) {
  SubmitDirection(EPacDirection::Up);
}

void APacPlayerController::HandleMoveDown(const FInputActionValue &Value) {
  SubmitDirection(EPacDirection::Down);
}

void APacPlayerController::HandleMoveLeft(const FInputActionValue &Value) {
  SubmitDirection(EPacDirection::Left);
}

void APacPlayerController::HandleMoveRight(const FInputActionValue &Value) {
  SubmitDirection(EPacDirection::Right);
}