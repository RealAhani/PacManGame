// clang-format off
#include "PacFruitPawn.h"
#include "PacFruitDecisionComponent.h"
#include "PacFruitDefinition.h"
#include "PacGhostMovementComponent.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
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

    // A dynamic instance per slot, so the flash can change a parameter without
    // touching the shared material. SetStaticMesh keeps the old overrides, and
    // CreateAndSetMaterialInstanceDynamic re-uses them, so the new mesh would
    // get the previous fruit's material. Clear them first: the instances are
    // then made from the new mesh's own materials.
    FlashMaterials.Reset();
    FruitMesh->EmptyOverrideMaterials();
    for (int32 Slot = 0; Slot < FruitMesh->GetNumMaterials(); ++Slot)
    {
        if (UMaterialInstanceDynamic* const
                Dynamic = FruitMesh->CreateAndSetMaterialInstanceDynamic(Slot))
        {
            FlashMaterials.Add(Dynamic);
        }
    }
    SetFlashAmount(0.0f);
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

void APacFruitPawn::SetFlashAmount(float const Amount)
{
    for (UMaterialInstanceDynamic* const Dynamic : FlashMaterials)
    {
        if (Dynamic)
        {
            Dynamic->SetScalarParameterValue(FlashParameterName, Amount);
        }
    }
}
