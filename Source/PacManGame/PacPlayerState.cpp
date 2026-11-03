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

int32 APacPlayerState::GetLives() const
{
    return Lives;
}

void APacPlayerState::SetLives(int32 const NewLives)
{
    Lives = FMath::Max(NewLives, 0);

    OnLivesChanged.Broadcast(Lives);
}

int32 APacPlayerState::LoseLife()
{
    SetLives(Lives - 1);

    UE_LOG(LogPacMan, Log, TEXT("PAC: Life lost. Lives=%d"), Lives);

    return Lives;
}
