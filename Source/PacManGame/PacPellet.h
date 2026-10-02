// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "PacGridTypes.h"
#include "GameFramework/Actor.h"
#include "PacPellet.generated.h"
// clang-format on

class USceneComponent;

UCLASS()
class PACMANGAME_API APacPellet : public AActor
{
    GENERATED_BODY()

public:

    // Sets default values for this actor's properties
    APacPellet();
    void Initialize(FPacGridPosition const & InGridPosition);

    FPacGridPosition GetGridPosition() const
    {
        return GridPosition;
    }
    int32 GetScoreValue() const
    {
        return ScoreValue;
    }

private:

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USceneComponent> BaseComponent;

    UPROPERTY(VisibleInstanceOnly, Category = "Pellet")
    FPacGridPosition GridPosition;

    UPROPERTY(EditDefaultsOnly, Category = "Pellet", meta = (ClampMin = "0"))
    int32 ScoreValue = 10;
};
