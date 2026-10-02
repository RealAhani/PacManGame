// clang-format off
#include "PacGhostPawn.h"
#include "Components/SphereComponent.h"
#include "PacGhostAIController.h"
#include "PacGhostDecisionComponent.h"
#include "PacGhostMovementComponent.h"
// clang-format on

APacGhostPawn::APacGhostPawn()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(
        TEXT("CollisionComponent"));

    SetRootComponent(CollisionComponent);

    CollisionComponent->SetSphereRadius(40.0f);

    GhostMovementComponent = CreateDefaultSubobject<UPacGhostMovementComponent>(
        TEXT("GhostMovementComponent"));

    GhostMovementComponent->SetUpdatedComponent(CollisionComponent);

    GhostDecisionComponent = CreateDefaultSubobject<UPacGhostDecisionComponent>(
        TEXT("GhostDecisionComponent"));

    AIControllerClass = APacGhostAIController::StaticClass();
    AutoPossessAI     = EAutoPossessAI::PlacedInWorldOrSpawned;
}

UPacGhostMovementComponent* APacGhostPawn::GetGhostMovementComponent() const
{
    return GhostMovementComponent;
}

UPacGhostDecisionComponent* APacGhostPawn::GetGhostDecisionComponent() const
{
    return GhostDecisionComponent;
}