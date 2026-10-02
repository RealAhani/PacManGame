#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PacGhostManager.generated.h"
// clang-format on

class APacMaze;
class APacGhostPawn;

UCLASS()
class PACMANGAME_API APacGhostManager : public AActor
{
    GENERATED_BODY()

public:

    APacGhostManager();

    void Initialize(APacMaze* InMaze);

private:

    void SpawnGhosts();

    UPROPERTY(EditDefaultsOnly, Category = "Ghosts")
    TSubclassOf<APacGhostPawn> GhostClass;

    UPROPERTY()
    TObjectPtr<APacMaze> Maze;

    UPROPERTY()
    TArray<TObjectPtr<APacGhostPawn>> Ghosts;
};