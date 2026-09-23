// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacHUD.h"
#include "PacHUDWidget.h"
#include "PacPlayerState.h"
#include "GameFramework/PlayerController.h"

#include "Blueprint/UserWidget.h"
// clang-format on

void APacHUD::BeginPlay() {
  Super::BeginPlay();

  if (HUDWidgetClass == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("PAC: HUDWidgetClass is not assigned."));
    return;
  }

  HUDWidget = CreateWidget<UPacHUDWidget>(GetWorld(), HUDWidgetClass);

  if (HUDWidget == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("PAC: Failed to create HUD widget."));
    return;
  }

  HUDWidget->AddToViewport();

  APlayerController *PlayerController = GetOwningPlayerController();

  if (PlayerController == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("PAC: HUD has no owning PlayerController."));
    return;
  }

  APacPlayerState *PacPlayerState =
      Cast<APacPlayerState>(PlayerController->PlayerState);

  if (PacPlayerState == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("PAC: HUD could not find APacPlayerState."));
    return;
  }

  // Set the initial value.
  HUDWidget->SetScore(PacPlayerState->GetScore());

  // Listen for future changes.
  PacPlayerState->OnScoreChanged.AddUObject(this, &APacHUD::HandleScoreChanged);
}

void APacHUD::HandleScoreChanged(const int32 Score) {
  if (HUDWidget == nullptr) {
    return;
  }

  HUDWidget->SetScore(Score);
}