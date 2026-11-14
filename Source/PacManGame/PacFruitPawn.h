#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PacFruitPawn.generated.h"
// clang-format on

class USphereComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UPacFruitDefinition;
class UPacGhostMovementComponent;
class UPacFruitDecisionComponent;
class UMaterialInstanceDynamic;

// The bonus fruit. One of these is created by APacFruitManager and re-used for
// every spawn: it is shown with ShowFruit() and hidden with HideFruit().
//
// It moves with the same grid movement as a ghost (UPacGhostMovementComponent,
// the ghost house stays closed to it). The mesh is a child component so it can
// float up and down without touching the grid position.
UCLASS()
class PACMANGAME_API APacFruitPawn : public APawn
{
    GENERATED_BODY()

public:

    APacFruitPawn();
    virtual void Tick(float DeltaSeconds) override;

    UPacGhostMovementComponent* GetFruitMovementComponent() const;
    UPacFruitDecisionComponent* GetFruitDecisionComponent() const;

    // Applies the definition (mesh, size, speed), shows the fruit and starts
    // the float animation. Mesh may be null: the fruit is then invisible.
    void ShowFruit(UPacFruitDefinition const & Definition, UStaticMesh* LoadedMesh);

    void HideFruit();

    // -1 = normal, 1 = fully white. Sets the scalar material parameter
    // FlashParameterName on the fruit's materials.
    void SetFlashAmount(float Amount);

    bool IsFruitShown() const
    {
        return bShown;
    }

private:

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USphereComponent> RootSphere;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UStaticMeshComponent> FruitMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UPacGhostMovementComponent> FruitMovementComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UPacFruitDecisionComponent> FruitDecisionComponent;

    // How high the fruit floats above its base height, and how fast.
    UPROPERTY(EditDefaultsOnly, Category = "Float", meta = (ClampMin = "0.0"))
    float BobHeight = 15.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Float", meta = (ClampMin = "0.0"))
    float BobSpeed = 4.0f;

    // Height of the mesh above the maze floor.
    UPROPERTY(EditDefaultsOnly, Category = "Float")
    float BaseHeight = 30.0f;

    // Name of the scalar parameter in the fruit materials (see the material
    // setup: Lerp(color, white, FlashAmount) and emissive).
    UPROPERTY(EditDefaultsOnly, Category = "Flash")
    FName FlashParameterName = TEXT("FlashAmount");

    // One dynamic instance per material slot, made in ShowFruit().
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> FlashMaterials;

    float BobTime = 0.0f;
    bool  bShown  = false;
};
