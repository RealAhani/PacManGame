// Fill out your copyright notice in the Description page of Project Settings.

#include "PacPlayerState.h"
#include "Logging/PacManLog.h"

int32 APacPlayerState::GetCurrentScore() const
{
    return CurrentScore;
}

void APacPlayerState::AddScore(int32 const Amount)
{
    CurrentScore += Amount;

    UE_LOG(LogPacMan, Log, TEXT("PAC: Score changed. Score=%d"), CurrentScore);
    OnScoreChanged.Broadcast(CurrentScore);
}
