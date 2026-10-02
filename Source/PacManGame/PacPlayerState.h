#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PacPlayerState.generated.h"

// -------------------- Delegate(Fptr-event) --------------------
DECLARE_MULTICAST_DELEGATE_OneParam(FOnScoreChanged, int32);

UCLASS()
class PACMANGAME_API APacPlayerState : public APlayerState
{
    GENERATED_BODY()

public:

    // -------------------- functions --------------------
    int32 GetScore() const;
    void  AddScore(int32 Amount);

    // -------------------- properties --------------------
    FOnScoreChanged OnScoreChanged;

private:

    UPROPERTY(VisibleInstanceOnly, Category = "Score")
    int32 Current_Score = 0;
};