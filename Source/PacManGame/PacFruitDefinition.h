#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "PacFruitDefinition.generated.h"
// clang-format on

// One kind of bonus fruit. Make one asset per fruit in the Content Browser
// (Miscellaneous > Data Asset > PacFruitDefinition) and add them to the
// Fruits list of the APacFruitManager placed in the level.
//
// It is a primary data asset so the Asset Manager can scan / bundle fruit
// later without changing any code.
UCLASS(BlueprintType)
class PACMANGAME_API UPacFruitDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("PacFruit")), GetFName());
    }

    // Soft reference: the mesh is only loaded when the manager preloads it, not
    // when this asset (or the level) loads.
    UPROPERTY(EditDefaultsOnly, Category = "Fruit")
    TSoftObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditDefaultsOnly, Category = "Fruit")
    FText DisplayName;

    // Original Pac-Man values: Cherry 100, Strawberry 300, Peach (orange) 500.
    UPROPERTY(EditDefaultsOnly, Category = "Fruit", meta = (ClampMin = "0"))
    int32 Score = 100;

    // Speed relative to the ghost movement speed. Below 1: slower than a ghost.
    UPROPERTY(EditDefaultsOnly, Category = "Fruit", meta = (ClampMin = "0.1"))
    float SpeedMultiplier = 0.6f;

    UPROPERTY(EditDefaultsOnly, Category = "Fruit", meta = (ClampMin = "0.01"))
    float MeshScale = 1.0f;

    // Chance relative to the other fruit. 0 = never.
    UPROPERTY(EditDefaultsOnly, Category = "Fruit", meta = (ClampMin = "0.0"))
    float SpawnWeight = 1.0f;
};
