#pragma once

// clang-format off
#include "PacGridTypes.h"

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "PacGhostMovementComponent.generated.h"
// clang-format on

class APacMaze;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGhostGridPositionChanged,
                                    FPacGridPosition const &);

UCLASS(ClassGroup = (PacMan), meta = (BlueprintSpawnableComponent))
class PACMANGAME_API UPacGhostMovementComponent : public UPawnMovementComponent
{
    GENERATED_BODY()

public:

    UPacGhostMovementComponent();
    void InitializeMaze(APacMaze* InMaze, FPacGridPosition const & StartPosition);
    void             SetDirection(EPacDirection Direction);
    FPacGridPosition GetCurrentGridPosition() const;
    EPacDirection    GetCurrentDirection() const;


    FOnGhostGridPositionChanged OnGridPositionChanged;

protected:

    virtual void TickComponent(float      DeltaTime,
                               ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:

    bool TryStartMovement();
    void MoveTowardTarget(float DeltaTime);

    UPROPERTY(EditInstanceOnly, Category = "Grid Movement")
    TObjectPtr<APacMaze> Maze;

    UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
    FPacGridPosition CurrentGridPosition;

    UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
    FPacGridPosition TargetGridPosition;

    UPROPERTY(VisibleInstanceOnly, Category = "Grid Movement")
    EPacDirection CurrentDirection = EPacDirection::None;

    UPROPERTY(EditDefaultsOnly, Category = "Grid Movement", meta = (ClampMin = "0.0"))
    float MovementSpeed = 250.0f;
};
