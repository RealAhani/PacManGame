// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PacGridTypes.h"
#include "PacMazeDataAsset.generated.h"
// clang-format on

/**
 *
 */
UCLASS(BlueprintType)
class PACMANGAME_API UPacMazeDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Maze")
    FIntPoint GridSize = FIntPoint(28, 31);

    UPROPERTY(EditDefaultsOnly,
              BlueprintReadOnly,
              Category = "Maze",
              meta     = (MultiLine = true))
    FString Layout;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Maze")
    FPacGridPosition PlayerStart;

    // Where each ghost starts. A start on an 'H' tile means that ghost starts
    // inside the ghost house and waits for its wave to come out.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Maze")
    TArray<FPacGridPosition> GhostStarts;

    // The street tile right outside the ghost house door (NOT an 'H' tile).
    // A ghost has left the house once it reaches this tile. Only needed
    // when the layout has 'H' tiles.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Maze")
    FPacGridPosition GhostHouseExit;
};
