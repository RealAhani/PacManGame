#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "Dev/PacDevConfig.h"
#include "PacFruitFlash.h"
#include "PacGridTypes.h"
#include "PacFruitManager.generated.h"
// clang-format on

class APacMaze;
class APacPelletManager;
class APacFruitPawn;
class UPacFruitDefinition;
class UPacGridMovementComponent;
class UNiagaraSystem;

// Fires with the fruit's score when Pac-Man eats it.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnFruitEaten, int32 /*Score*/);

// Owns the bonus fruit: when it appears, which fruit, where, how long it stays,
// and eating it. Place one in the level (like the pellet and ghost managers)
// and fill in Fruits. Without one the game simply has no fruit.
UCLASS()
class PACMANGAME_API APacFruitManager : public AActor
{
    GENERATED_BODY()

public:

    APacFruitManager();
    virtual void Tick(float DeltaSeconds) override;

    // Called by the GameMode (see APacGameMode::InitializeLevel).
    void Initialize(APacMaze*                  InMaze,
                    APacPelletManager*         InPelletManager,
                    UPacGridMovementComponent* InPlayerMovement);

    // Freezes / unfreezes the fruit together with Pac-Man and the ghosts.
    void SetMovementEnabled(bool bEnabled);

    // Takes the fruit off the maze (after a death). The pellet count is kept,
    // so a fruit that already appeared does not come back for the same pellets.
    void ResetFruit();

    // Eats the fruit if Pac-Man is within HitDistance of it (maze plane, world
    // units). Fires OnFruitEaten and returns true when it was eaten.
    bool UpdateContacts(FVector const & PlayerLocation, float HitDistance);

    bool IsFruitActive() const;

    FOnFruitEaten OnFruitEaten;

#if PAC_DEV
    // Cheat: show a fruit right now.
    void SpawnFruitNow()
    {
        SpawnFruit();
    }
#endif

private:

    // -------------------- functions --------------------
    void                 HandlePelletConsumed(int32 PelletScore);
    void                 SpawnFruit();
    void                 HideActiveFruit();
    void                 PlayVanishEffect() const;
    bool                 EnsureFruitPawn();
    bool                 PickSpawnTile(FPacGridPosition & OutTile) const;
    UPacFruitDefinition* PickDefinition() const;
    void                 PreloadMeshes();

    // -------------------- properties --------------------
    // The kinds of fruit: Cherry, Strawberry, Peach...
    UPROPERTY(EditAnywhere, Category = "Fruit")
    TArray<TObjectPtr<UPacFruitDefinition>> Fruits;

    UPROPERTY(EditAnywhere, Category = "Fruit")
    TSubclassOf<APacFruitPawn> FruitPawnClass;

    // A fruit appears when this many pellets have been eaten (original: 70, 170).
    UPROPERTY(EditAnywhere, Category = "Fruit")
    TArray<int32> SpawnAtPelletCounts = {70, 170};

    // How long a fruit stays if nobody eats it (original: about 10 seconds).
    UPROPERTY(EditAnywhere, Category = "Fruit", meta = (ClampMin = "1.0"))
    float FruitLifetimeSeconds = 10.0f;

    // The fruit appears at least this many steps away from Pac-Man.
    UPROPERTY(EditAnywhere, Category = "Fruit", meta = (ClampMin = "0"))
    int32 MinSpawnDistanceInTiles = 8;

    // The white flashing before the fruit leaves (starts with 19% of the
    // lifetime left, faster every 3%).
    UPROPERTY(EditAnywhere, Category = "Fruit|Flash")
    FPacFruitFlashSettings Flash;

    // Played where the fruit was when it leaves the maze uneaten (the white
    // cube poof). Optional: empty = the fruit just disappears.
    UPROPERTY(EditAnywhere, Category = "Fruit|Effects")
    TObjectPtr<UNiagaraSystem> VanishEffect;


    UPROPERTY()
    TObjectPtr<APacMaze> Maze;

    UPROPERTY()
    TObjectPtr<APacPelletManager> PelletManager;

    UPROPERTY()
    TObjectPtr<UPacGridMovementComponent> PlayerMovement;

    UPROPERTY()
    TObjectPtr<APacFruitPawn> FruitPawn;

    UPROPERTY()
    TObjectPtr<UPacFruitDefinition> ActiveDefinition;

    UPROPERTY()
    TObjectPtr<UPacFruitDefinition> LastDefinition;

    // Keeps the preloaded meshes in memory. If this handle is released the
    // meshes can be unloaded again.
    TSharedPtr<FStreamableHandle> MeshLoadHandle;

    int32 PelletsEaten = 0;
    float LifeTimeLeft = 0.0f;
    float FlashPhase   = 0.0f;
    bool  bRunning     = false;
};
