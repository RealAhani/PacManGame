#pragma once

// clang-format off
#include "PacGridTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "PacGhostDecisionComponent.generated.h"
// clang-format on

class APacMaze;
class UPacGhostMovementComponent;

UCLASS(ClassGroup = (PacMan), meta = (BlueprintSpawnableComponent))
class PACMANGAME_API UPacGhostDecisionComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UPacGhostDecisionComponent();

    void Initialize(APacMaze*                   InMaze,
                    UPacGhostMovementComponent* InMovementComponent);

private:

    void HandleGridPositionChanged(FPacGridPosition const & Position);

    void ChooseDirection();

    bool IsDirectionWalkable(FPacGridPosition const & Position,
                             EPacDirection            Direction) const;

    UPROPERTY()
    TObjectPtr<APacMaze> Maze;

    UPROPERTY()
    TObjectPtr<UPacGhostMovementComponent> MovementComponent;
};