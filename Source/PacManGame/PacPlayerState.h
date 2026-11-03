#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PacPlayerState.generated.h"

// -------------------- Delegate(Fptr-event) --------------------
DECLARE_MULTICAST_DELEGATE_OneParam(FOnScoreChanged, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLivesChanged, int32);

UCLASS()
class PACMANGAME_API APacPlayerState : public APlayerState
{
    GENERATED_BODY()

public:

    // -------------------- functions --------------------
    int32 GetCurrentScore() const;
    void  AddScore(int32 Amount);
    int32 GetLives() const;
    void  SetLives(int32 NewLives);
    // Removes one life (never below zero) and returns how many are left.
    int32 LoseLife();


    // -------------------- properties --------------------
    FOnScoreChanged OnScoreChanged;
    FOnLivesChanged OnLivesChanged;

private:

    UPROPERTY(VisibleInstanceOnly, Category = "Score")
    int32 CurrentScore = 0;

    UPROPERTY(VisibleInstanceOnly, Category = "Score")
    int32 Lives = 3;
};
