// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PacGameMode.generated.h"
// clang-format on

class APacMaze;
class APacPelletManager;
class APacPlayerState;
class APacGhostManager;

UCLASS()
class PACMANGAME_API APacGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:

    APacGameMode();
    virtual void       BeginPlay() override;
    APacMaze*          GetMaze() const;
    APacPelletManager* GetPelletManager() const;
    APacGhostManager*  GetGhostManager() const;

private:

    void HandleAllPelletsConsumed();
    // Wires the whole level together in ONE fixed order (see the .cpp).
    // Nothing else may depend on BeginPlay order between actors.
    bool InitializeLevel();
    void HandlePelletConsumed(int32 ScoreValue);

    UPROPERTY()
    TObjectPtr<APacMaze> Maze;

    UPROPERTY()
    TObjectPtr<APacPelletManager> PelletManager;

    UPROPERTY()
    TObjectPtr<APacGhostManager> GhostManager;
};
