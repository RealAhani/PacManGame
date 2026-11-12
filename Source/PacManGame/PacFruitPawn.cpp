// clang-format off
#include "PacFruitPawn.h"
#include "PacFruitDecisionComponent.h"
#include "PacFruitDefinition.h"
#include "PacGhostMovementComponent.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
// clang-format on

APacFruitPawn::APacFruitPawn()
{
    PrimaryActorTick.bCanEverTick          = true;
    PrimaryActorTick.bStartWithTickEnabled = false;  // only while shown

    // Contact with Pac-Man is a distance test (see APacFruitManager), so the
    // fruit needs no collision at all.
    RootSphere = CreateDefaultSubobject<USphereComponent>(TEXT("RootSphere"));
    RootSphere->SetSphereRadius(30.0f);
    RootSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetRootComponent(RootSphere);

    FruitMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FruitMesh"));
    FruitMesh->SetupAttachment(RootSphere);
    FruitMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FruitMesh->SetRelativeLocation(FVector(0.0f, 0.0f, BaseHeight));

    FruitMovementComponent = CreateDefaultSubobject<UPacGhostMovementComponent>(
        TEXT("FruitMovementComponent"));
    FruitMovementComponent->SetUpdatedComponent(RootSphere);

    FruitDecisionComponent = CreateDefaultSubobject<UPacFruitDecisionComponent>(
        TEXT("FruitDecisionComponent"));

    // No AI controller: the decision component drives the movement.
    AutoPossessAI = EAutoPossessAI::Disabled;

    SetActorHiddenInGame(true);
}

void APacFruitPawn::Tick(float const DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Float up and down. Only the mesh moves, never the grid position.
    BobTime += DeltaSeconds;

    FruitMesh->SetRelativeLocation(
        FVector(0.0f, 0.0f, BaseHeight + BobHeight * FMath::Sin(BobTime * BobSpeed)));
}

UPacGhostMovementComponent* APacFruitPawn::GetFruitMovementComponent() const
{
    return FruitMovementComponent;
}

UPacFruitDecisionComponent* APacFruitPawn::GetFruitDecisionComponent() const
{
    return FruitDecisionComponent;
}

void APacFruitPawn::ShowFruit(UPacFruitDefinition const & Definition,
                              UStaticMesh*                LoadedMesh)
{
    FruitMesh->SetStaticMesh(LoadedMesh);
    FruitMesh->SetRelativeScale3D(FVector(Definition.MeshScale));
    FruitMovementComponent->SetSpeedMultiplier(Definition.SpeedMultiplier);

    BobTime = 0.0f;
    bShown  = true;

    SetActorHiddenInGame(false);
    SetActorTickEnabled(true);
}

void APacFruitPawn::HideFruit()
{
    bShown = false;

    FruitMovementComponent->SetMovementEnabled(false);

    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
}
