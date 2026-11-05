#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "PacCheatManager.generated.h"

UCLASS()
class PACMANGAME_API UPacCheatManager : public UCheatManager
{
    GENERATED_BODY()

public:

    UFUNCTION(Exec)
    void PacGodMode(bool bEnabled);

    // UFUNCTION(Exec)
    // void PacPlayerSpeed(float Multiplier);

    // UFUNCTION(Exec)
    // void PacGhostsFrightened();

    UFUNCTION(Exec)
    void PacGhostsKillAll();

    UFUNCTION(Exec)
    void PacLose();

    UFUNCTION(Exec)
    void PacWin();
};
