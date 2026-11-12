#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PacGridTypes.h"
#include "PacFruitDecisionComponent.generated.h"
// clang-format on

class APacMaze;
class UPacGhostMovementComponent;
class UPacGridMovementComponent;

// The fruit's brain: every time the fruit reaches a tile it picks the next
// direction, running away from Pac-Man.
//
// It runs to the tile that is farthest from Pac-Man among the tiles the fruit
// can reach before Pac-Man can (FPacGrid::FindFleeTarget). If there is none, it
// wanders. It never turns straight back unless it is in a dead end.
UCLASS(ClassGroup = (PacMan), meta = (BlueprintSpawnableComponent))
class PACMANGAME_API UPacFruitDecisionComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UPacFruitDecisionComponent();

    // Call once, after the movement component knows the maze.
    void Initialize(APacMaze*                   InMaze,
                    UPacGhostMovementComponent* InMovementComponent,
                    UPacGridMovementComponent*  InPlayerMovementComponent);

    // Call after the fruit was put on its spawn tile.
    void Respawn();

private:

    void HandleGridPositionChanged([[maybe_unused]] FPacGridPosition const & Position);
    void ChooseDirection();
    bool ChooseRandomDirection();

    UPROPERTY()
    TObjectPtr<APacMaze> Maze;

    UPROPERTY()
    TObjectPtr<UPacGhostMovementComponent> MovementComponent;

    UPROPERTY()
    TObjectPtr<UPacGridMovementComponent> PlayerMovementComponent;
};
