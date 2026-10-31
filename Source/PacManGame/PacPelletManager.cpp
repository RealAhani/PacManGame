// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacPelletManager.h"
#include "PacMaze.h"
#include "PacGridMovementComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
// clang-format on

// Sets default values
APacPelletManager::APacPelletManager()
{
    PrimaryActorTick.bCanEverTick = false;
    NormalPelletInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("NormalPelletInstances"));

    SetRootComponent(NormalPelletInstances);

    PowerPelletInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("PowerPelletInstances"));

    PowerPelletInstances->SetupAttachment(RootComponent);

    NormalPelletInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    PowerPelletInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


void APacPelletManager::Initialize(APacMaze* InMaze)
{
    check(InMaze);

    Maze = InMaze;

    SpawnPellets();
}

void APacPelletManager::SpawnPellets()
{
    check(Maze);
    // this->SetActorLocationAndRotation(Maze->GetActorLocation() - Maze->WallExtent,
    //                                   Maze->GetActorRotation());
    if (NormalPelletInstances->GetStaticMesh() == nullptr ||
        PowerPelletInstances->GetStaticMesh() == nullptr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("PAC: PelletManager is missing an ISM static mesh."));
        return;
    }

    Pellets.Empty();

    NormalPelletInstances->ClearInstances();
    PowerPelletInstances->ClearInstances();

    FIntPoint const GridSize = Maze->GetGridSize();

    for (int32 Y = 0; Y < GridSize.Y; ++Y)
    {
        for (int32 X = 0; X < GridSize.X; ++X)
        {
            FPacGridPosition const Position(X, Y);

            EPacTileType const TileType = Maze->GetTile(Position);

            bool  bIsPowerPellet = false;
            int32 ScoreValue     = 0;

            switch (TileType)
            {
                case EPacTileType::Pellet:
                    ScoreValue = NormalPelletScore;
                    break;

                case EPacTileType::PowerPellet:
                    bIsPowerPellet = true;
                    ScoreValue     = PowerPelletScore;
                    break;

                default:
                    continue;
            }
            FVector InstanceScale = bIsPowerPellet ? FVector(0.3f) : FVector(1.0f);

            FTransform InstanceTransform;
            InstanceTransform.SetLocation(Maze->GridToWorld(Position));
            InstanceTransform.SetRotation(FQuat::Identity);
            InstanceTransform.SetScale3D(InstanceScale);


            UInstancedStaticMeshComponent* Instances = bIsPowerPellet
                                                           ? PowerPelletInstances
                                                           : NormalPelletInstances;

            int32 const InstanceIndex = Instances->AddInstance(InstanceTransform, true);
            // int32 const
            //     InstanceIndex = Instances->AddInstance(FTransform(FRotator::ZeroRotator,
            //                                                       Maze->GridToWorld(
            //                                                           Position)),
            //                                            true);

            FPacPelletData PelletData;
            PelletData.GridPosition   = Position;
            PelletData.ScoreValue     = ScoreValue;
            PelletData.InstanceIndex  = InstanceIndex;
            PelletData.bIsPowerPellet = bIsPowerPellet;

            Pellets.Add(Position, PelletData);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("PAC: Created %d pellet instances."), Pellets.Num());
}

void APacPelletManager::HandlePlayerGridPositionChanged(FPacGridPosition const & Position)
{
    FPacPelletData* FoundPellet = Pellets.Find(Position);

    if (FoundPellet == nullptr)
    {
        return;
    }

    FPacPelletData const PelletData = *FoundPellet;

    UInstancedStaticMeshComponent* Instances = PelletData.bIsPowerPellet
                                                   ? PowerPelletInstances
                                                   : NormalPelletInstances;

    if (!IsValid(Instances) || !Instances->IsValidInstance(PelletData.InstanceIndex))
    {
        UE_LOG(LogTemp,
               Warning,
               TEXT("PAC: Invalid pellet instance at (%d,%d)."),
               Position.X,
               Position.Y);

        Pellets.Remove(Position);
        return;
    }

    Instances->RemoveInstance(PelletData.InstanceIndex);

    Pellets.Remove(Position);

    // Removing an ISM instance can shift the indices of later instances.
    // Update the stored indices for the affected pellet type.
    for (TPair<FPacGridPosition, FPacPelletData> & Pair : Pellets)
    {
        FPacPelletData & OtherPellet = Pair.Value;

        if (OtherPellet.bIsPowerPellet == PelletData.bIsPowerPellet &&
            OtherPellet.InstanceIndex > PelletData.InstanceIndex)
        {
            --OtherPellet.InstanceIndex;
        }
    }

    OnPelletConsumed.Broadcast(PelletData.ScoreValue);

    if (Pellets.IsEmpty())
    {
        OnAllPelletsConsumed.Broadcast();
    }

    UE_LOG(LogTemp,
           Log,
           TEXT("PAC: Pellet consumed at (%d,%d). Value=%d, Remaining=%d"),
           Position.X,
           Position.Y,
           PelletData.ScoreValue,
           Pellets.Num());
}

void APacPelletManager::BindToPlayerMovement(UPacGridMovementComponent* MovementComponent)
{
    check(MovementComponent);

    MovementComponent->OnGridPositionChanged
        .AddUObject(this, &APacPelletManager::HandlePlayerGridPositionChanged);
}
