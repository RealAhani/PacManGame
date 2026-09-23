// Fill out your copyright notice in the Description page of Project Settings.

#include "PacPlayerState.h"

int32 APacPlayerState::GetScore() const { return Current_Score; }

void APacPlayerState::AddScore(const int32 Amount) {
  Current_Score += Amount;

  UE_LOG(LogTemp, Log, TEXT("PAC: Score changed. Score=%d"), Current_Score);
  OnScoreChanged.Broadcast(Current_Score);
}
