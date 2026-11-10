#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PacGridTypes.h"
#include "PacPelletManager.generated.h"
// clang-format on

class APacMaze;
class UPacGridMovementComponent;
class UInstancedStaticMeshComponent;

// -------------------- Delegate(Fptr,event) --------------------
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPelletConsumed, int32);
DECLARE_MULTICAST_DELEGATE(FOnAllPelletsConsumed);
DECLARE_MULTICAST_DELEGATE(FOnPowerPelletConsumed);

struct FPacPelletData
{
    FPacGridPosition GridPosition;
    bool             bIsPowerPellet = false;
    int32            ScoreValue     = 0;
    int32            InstanceIndex  = INDEX_NONE;
};

UCLASS()
class PACMANGAME_API APacPelletManager : public AActor
{
    GENERATED_BODY()

public:

    // -------------------- functions --------------------
    APacPelletManager();
    void Initialize(APacMaze* InMaze);
    void BindToPlayerMovement(UPacGridMovementComponent* MovementComponent);

    // -------------------- properties --------------------
    FOnPelletConsumed      OnPelletConsumed;
    FOnAllPelletsConsumed  OnAllPelletsConsumed;
    FOnPowerPelletConsumed OnPowerPelletConsumed;

private:

    // -------------------- functions --------------------
    void SpawnPellets();
    void HandlePlayerGridPositionChanged(FPacGridPosition const & Position);

    // -------------------- properties --------------------

    UPROPERTY(VisibleAnywhere, Category = "Pellets")
    TObjectPtr<UInstancedStaticMeshComponent> NormalPelletInstances;

    UPROPERTY(VisibleAnywhere, Category = "Pellets")
    TObjectPtr<UInstancedStaticMeshComponent> PowerPelletInstances;

    UPROPERTY(EditDefaultsOnly, Category = "Pellets", meta = (ClampMin = "0"))
    int32 NormalPelletScore = 10;

    UPROPERTY(EditDefaultsOnly, Category = "Pellets", meta = (ClampMin = "0"))
    int32 PowerPelletScore = 50;

    UPROPERTY()
    TObjectPtr<APacMaze> Maze;


    UPROPERTY(EditAnywhere,
              Category = "Pellets",
              meta     = (ClampMin = "0.1", ClampMax = "1.0"))
    float PelletSize = 1.0f;

    UPROPERTY(EditAnywhere,
              Category = "Pellets",
              meta     = (ClampMin = "0.1", ClampMax = "1.0"))
    float PowerPelletSize = 1.0f;

    TMap<FPacGridPosition, FPacPelletData> Pellets;
};
