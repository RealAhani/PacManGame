// clang-format off
#include "PacFruitManager.h"
#include "Logging/PacManLog.h"
#include "PacFruitDecisionComponent.h"
#include "PacFruitDefinition.h"
#include "PacFruitPawn.h"
#include "PacGhostMovementComponent.h"
#include "PacGridMovementComponent.h"
#include "PacMaze.h"
#include "PacPelletManager.h"

#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
// clang-format on

APacFruitManager::APacFruitManager()
{
    PrimaryActorTick.bCanEverTick = true;

    FruitPawnClass = APacFruitPawn::StaticClass();

    // Something for the editor to show; the manager has no visuals of its own.
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void APacFruitManager::Initialize(APacMaze*                  InMaze,
                                  APacPelletManager*         InPelletManager,
                                  UPacGridMovementComponent* InPlayerMovement)
{
    check(InMaze);
    check(InPelletManager);
    check(InPlayerMovement);

    Maze           = InMaze;
    PelletManager  = InPelletManager;
    PlayerMovement = InPlayerMovement;
    PelletsEaten   = 0;

    if (Fruits.IsEmpty())
    {
        UE_LOG(LogPacMan,
               Warning,
               TEXT("PAC: FruitManager has an empty Fruits list: no fruit will "
                    "appear."));
    }

    PreloadMeshes();

    PelletManager->OnPelletConsumed
        .AddUObject(this, &APacFruitManager::HandlePelletConsumed);
}

// Starts loading every fruit mesh now (level start), so the first spawn does
// not have to wait for the disk.
void APacFruitManager::PreloadMeshes()
{
    TArray<FSoftObjectPath> Paths;

    for (UPacFruitDefinition const* Definition : Fruits)
    {
        if (Definition != nullptr && !Definition->Mesh.IsNull())
        {
            Paths.AddUnique(Definition->Mesh.ToSoftObjectPath());
        }
    }

    if (Paths.IsEmpty())
    {
        return;
    }

    MeshLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths);
}

void APacFruitManager::Tick(float const DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bRunning || !IsFruitActive())
    {
        return;
    }

    LifeTimeLeft -= DeltaSeconds;

    if (LifeTimeLeft <= 0.0f)
    {
        UE_LOG(LogPacMan, Log, TEXT("PAC: Fruit left the maze, nobody ate it."));
        HideActiveFruit();
        return;
    }
    float const Remaining = LifeTimeLeft / FruitLifetimeSeconds;

    FruitPawn->SetFlashAmount(Flash.Advance(Remaining, DeltaSeconds, FlashPhase));
}

void APacFruitManager::SetMovementEnabled(bool const bEnabled)
{
    bRunning = bEnabled;

    if (IsFruitActive())
    {
        FruitPawn->GetFruitMovementComponent()->SetMovementEnabled(bEnabled);
    }
}

void APacFruitManager::ResetFruit()
{
    HideActiveFruit();
}

bool APacFruitManager::IsFruitActive() const
{
    return FruitPawn != nullptr && FruitPawn->IsFruitShown();
}

void APacFruitManager::HideActiveFruit()
{
    if (FruitPawn != nullptr)
    {
        FruitPawn->HideFruit();
    }

    ActiveDefinition = nullptr;
}

bool APacFruitManager::UpdateContacts(FVector const & PlayerLocation,
                                      float const     HitDistance)
{
    if (!bRunning || !IsFruitActive())
    {
        return false;
    }

    if (FVector::Dist2D(FruitPawn->GetActorLocation(), PlayerLocation) > HitDistance)
    {
        return false;
    }

    int32 const Score = ActiveDefinition != nullptr ? ActiveDefinition->Score : 0;

    UE_LOG(LogPacMan, Log, TEXT("PAC: Fruit eaten. Score=%d"), Score);

    HideActiveFruit();
    OnFruitEaten.Broadcast(Score);

    return true;
}

void APacFruitManager::HandlePelletConsumed([[maybe_unused]] int32 const PelletScore)
{
    // One broadcast per eaten pellet (normal or power).
    ++PelletsEaten;

    if (SpawnAtPelletCounts.Contains(PelletsEaten))
    {
        SpawnFruit();
    }
}

bool APacFruitManager::EnsureFruitPawn()
{
    if (FruitPawn != nullptr)
    {
        return true;
    }

    if (FruitPawnClass == nullptr)
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: FruitManager has no FruitPawnClass."));
        return false;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters
        .SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    FruitPawn = GetWorld()->SpawnActor<APacFruitPawn>(FruitPawnClass,
                                                      Maze->GetActorLocation(),
                                                      FRotator::ZeroRotator,
                                                      SpawnParameters);

    if (FruitPawn == nullptr)
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: Failed to spawn the fruit pawn."));
        return false;
    }

    FruitPawn->GetFruitDecisionComponent()
        ->Initialize(Maze, FruitPawn->GetFruitMovementComponent(), PlayerMovement);

    return true;
}

void APacFruitManager::SpawnFruit()
{
    if (Maze == nullptr || PlayerMovement == nullptr)
    {
        return;
    }

    UPacFruitDefinition* const Definition = PickDefinition();

    if (Definition == nullptr)
    {
        UE_LOG(LogPacMan,
               Warning,
               TEXT("PAC: No fruit to spawn (empty or all weights 0)."));
        return;
    }

    FPacGridPosition SpawnTile;

    if (!PickSpawnTile(SpawnTile))
    {
        UE_LOG(LogPacMan, Warning, TEXT("PAC: No free tile for the fruit."));
        return;
    }

    if (!EnsureFruitPawn())
    {
        return;
    }

    // The mesh was requested at level start. If it is somehow not there yet,
    // load it now instead of showing an empty fruit.
    UStaticMesh* Mesh = Definition->Mesh.Get();

    if (Mesh == nullptr && !Definition->Mesh.IsNull())
    {
        UE_LOG(LogPacMan,
               Warning,
               TEXT("PAC: Fruit mesh of '%s' was not preloaded, loading it "
                    "now."),
               *Definition->GetName());

        Mesh = Definition->Mesh.LoadSynchronous();
    }

    UPacGhostMovementComponent* const Movement = FruitPawn->GetFruitMovementComponent();

    // Puts the pawn on the tile and clears its old direction.
    Movement->InitializeMaze(Maze, SpawnTile);
    Movement->SetMovementEnabled(bRunning);

    FruitPawn->ShowFruit(*Definition, Mesh);
    FruitPawn->GetFruitDecisionComponent()->Respawn();

    ActiveDefinition = Definition;
    LastDefinition   = Definition;
    LifeTimeLeft     = FruitLifetimeSeconds;
    FlashPhase       = 0.0f;

    UE_LOG(LogPacMan,
           Log,
           TEXT("PAC: Fruit '%s' appeared at (%d,%d) after %d pellets."),
           *Definition->GetName(),
           SpawnTile.X,
           SpawnTile.Y,
           PelletsEaten);
}

// Weighted random, but never the same fruit twice in a row (if there is a
// choice).
UPacFruitDefinition* APacFruitManager::PickDefinition() const
{
    TArray<UPacFruitDefinition*> Candidates;

    for (UPacFruitDefinition* Definition : Fruits)
    {
        if (Definition != nullptr && Definition->SpawnWeight > 0.0f)
        {
            Candidates.Add(Definition);
        }
    }

    if (Candidates.Num() > 1)
    {
        Candidates.Remove(LastDefinition.Get());
    }

    float TotalWeight = 0.0f;
    for (UPacFruitDefinition const* Definition : Candidates)
    {
        TotalWeight += Definition->SpawnWeight;
    }

    if (Candidates.IsEmpty() || TotalWeight <= 0.0f)
    {
        return nullptr;
    }

    float Roll = FMath::FRandRange(0.0f, TotalWeight);

    for (UPacFruitDefinition* Definition : Candidates)
    {
        Roll -= Definition->SpawnWeight;

        if (Roll <= 0.0f)
        {
            return Definition;
        }
    }

    return Candidates.Last();
}

// A random walkable tile that Pac-Man can reach, away from him, not in the
// ghost house and not on the map edge (the tunnel mouths).
bool APacFruitManager::PickSpawnTile(FPacGridPosition & OutTile) const
{
    FPacGrid const & Grid = Maze->GetGrid();

    FPacGridPosition const PlayerTile = PlayerMovement->GetCurrentGridPosition();

    TArray<int32> PlayerField;
    Grid.BuildDistanceField(PlayerTile, PlayerField);

    FIntPoint const Size = Grid.GetSize();

    TArray<FPacGridPosition> FarTiles;
    TArray<FPacGridPosition> AnyTiles;

    for (int32 Y = 0; Y < Size.Y; ++Y)
    {
        for (int32 X = 0; X < Size.X; ++X)
        {
            FPacGridPosition const Tile(X, Y);

            if (!Grid.IsWalkable(Tile))
            {
                continue;
            }

            int32 const Distance = Grid.GetFieldDistance(PlayerField, Tile);

            // Pac-Man's own tile, or a tile he cannot reach.
            if (Distance <= 0)
            {
                continue;
            }

            if (X == 0 || Y == 0 || X == Size.X - 1 || Y == Size.Y - 1)
            {
                continue;
            }

            AnyTiles.Add(Tile);

            if (Distance >= MinSpawnDistanceInTiles)
            {
                FarTiles.Add(Tile);
            }
        }
    }

    // A tiny maze may have no tile that far away: then any tile will do.
    TArray<FPacGridPosition> const & Pool = FarTiles.IsEmpty() ? AnyTiles : FarTiles;

    if (Pool.IsEmpty())
    {
        return false;
    }

    OutTile = Pool[FMath::RandRange(0, Pool.Num() - 1)];
    return true;
}
