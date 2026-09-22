#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PacPlayerState.generated.h"

UCLASS()
class PACMANGAME_API APacPlayerState : public APlayerState {
  GENERATED_BODY()

public:
  int32 GetScore() const;

  void AddScore(int32 Amount);

private:
  UPROPERTY(VisibleInstanceOnly, Category = "Score")
  int32 Current_Score = 0;
};