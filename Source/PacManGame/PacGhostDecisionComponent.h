#pragma once

// clang-format off
#include "PacGridTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "PacGhostDecisionComponent.generated.h"
// clang-format on

class APacMaze;
class UPacGhostMovementComponent;
class UPacGridMovementComponent;

UENUM(BlueprintType)
enum class EPacGhostBehaviorMode : uint8
{
    None,
    Scatter,
    Chase,
    Frightened,
    Eaten
};

UENUM(BlueprintType)
enum class EPacGhostPersonality : uint8
{
    Blinky,
    Pinky,
    Inky,
    Clyde
};

UCLASS(ClassGroup = (PacMan), meta = (BlueprintSpawnableComponent))
class PACMANGAME_API UPacGhostDecisionComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UPacGhostDecisionComponent();

    void Initialize(APacMaze*                   InMaze,
                    UPacGhostMovementComponent* InMovementComponent);
    void SetScatterTarget(FPacGridPosition TargetPosition);
    void SetPlayerMovementComponent(UPacGridMovementComponent* InPlayerMovementComponent);
    void SetPersonality(EPacGhostPersonality NewPersonality);
    void SetBlinkyMovementComponent(UPacGhostMovementComponent* InBlinkyMovementComponent);


    UFUNCTION(BlueprintCallable, Category = "Ghost AI")
    void SetBehaviorMode(EPacGhostBehaviorMode NewMode);


private:

    void HandleGridPositionChanged(FPacGridPosition const & Position);

    void ChooseDirection();

    bool ChooseDirectionTowardTarget(FPacGridPosition const & TargetPosition);

    bool TryResolveWalkableTarget(FPacGridPosition const & DesiredTarget,
                                  FPacGridPosition &       OutTarget) const;

    bool IsDirectionWalkable(FPacGridPosition const & Position,
                             EPacDirection            Direction) const;

    void HandlePlayerGridPositionChanged(FPacGridPosition const & Position);

    UPROPERTY(VisibleInstanceOnly, Category = "Ghost AI")
    EPacGhostPersonality Personality = EPacGhostPersonality::Blinky;

    UPROPERTY()
    TObjectPtr<APacMaze> Maze;

    UPROPERTY()
    TObjectPtr<UPacGhostMovementComponent> MovementComponent;

    UPROPERTY()
    TObjectPtr<UPacGhostMovementComponent> BlinkyMovementComponent;

    UPROPERTY()
    TObjectPtr<UPacGridMovementComponent> PlayerMovementComponent;

    UPROPERTY(VisibleInstanceOnly, Category = "Ghost AI")
    EPacGhostBehaviorMode BehaviorMode = EPacGhostBehaviorMode::None;

    UPROPERTY(VisibleInstanceOnly, Category = "Ghost AI")
    FPacGridPosition ScatterTargetPosition;

    UPROPERTY(VisibleInstanceOnly, Category = "Ghost AI")
    bool bHasScatterTarget = false;
};
