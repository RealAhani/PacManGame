#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/StreamableManager.h"
#include "PacWallBuilderComponent.generated.h"
// clang-format on

struct FPacGrid;
class UInstancedStaticMeshComponent;
class UPacWallStyle;

// Builds the maze walls from the grid. Every wall tile looks at its 8
// neighbours (FPacGrid::TryGetWallPlacement), becomes one wall piece turned by 0
// to 3 quarter turns, and is added as an instance of that piece's mesh.
//
// Only the meshes the maze really uses are loaded, and they load in the
// background (soft references). One instanced static mesh component is made per
// mesh, so the whole maze costs a handful of draw calls.
//
// It is a component of APacMaze, which calls BuildWalls() when the grid is ready.
UCLASS(ClassGroup = (PacMan), meta = (BlueprintSpawnableComponent))
class PACMANGAME_API UPacWallBuilderComponent : public USceneComponent
{
    GENERATED_BODY()

public:

    UPacWallBuilderComponent();

    // Works out where every wall piece goes and starts loading the meshes. The
    // instances appear when the meshes have loaded. Replaces earlier walls.
    void BuildWalls(FPacGrid const & Grid, float TileSize);

    // Removes all wall instances.
    void ClearWalls();

private:

    void HandleMeshesLoaded();

    // -------------------- properties --------------------
    UPROPERTY(EditAnywhere,
              BlueprintReadOnly,
              Category = "Walls",
              meta     = (AllowPrivateAccess = "true"))
    TObjectPtr<UPacWallStyle> Style;

    // The tiles just outside the grid count as walls, so the outer wall ring
    // looks like part of a bigger wall instead of a free-standing line.
    UPROPERTY(EditAnywhere,
              BlueprintReadOnly,
              Category = "Walls",
              meta     = (AllowPrivateAccess = "true"))
    bool bOutsideIsWall = true;

    UPROPERTY()
    TArray<TObjectPtr<UInstancedStaticMeshComponent>> WallComponents;

    // Instance transforms (relative to this component) for each mesh, waiting
    // for that mesh to load.
    TMap<FSoftObjectPath, TArray<FTransform>> PendingInstances;

    // Keeps the loaded meshes in memory (and tells us when they are ready).
    TSharedPtr<FStreamableHandle> LoadHandle;
};
