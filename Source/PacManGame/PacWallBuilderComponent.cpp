// clang-format off
#include "PacWallBuilderComponent.h"
#include "Grid/PacGrid.h"
#include "Logging/PacManLog.h"
#include "PacWallStyle.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
// clang-format on

namespace
{
// Where one piece goes, relative to the maze: the tile centre, turned by the
// piece's quarter turns, with the style's scale and offsets applied.
FTransform MakeWallTransform(FPacGridPosition const &     Position,
                             FPacWallPlacement const &    Placement,
                             FPacWallPieceSetting const & Setting,
                             UPacWallStyle const &        Style,
                             float const                  TileSize)
{
    float const TileScale = TileSize / FMath::Max(Style.ReferenceTileSize, 1.0f);

    // One clockwise quarter turn (seen from above) is -90 degrees of yaw.
    float const TurnYaw = -90.0f * static_cast<float>(Placement.QuarterTurns);

    FVector const TileCentre(static_cast<float>(Position.X) * TileSize,
                             static_cast<float>(Position.Y) * TileSize,
                             0.0f);

    // The offset is written for the base orientation, so it turns with the piece
    // (but not with the extra YawOffset, which only fixes the mesh itself).
    FVector const Offset = FRotator(0.0f, TurnYaw, 0.0f)
                               .RotateVector(Setting.Offset * TileScale);

    FRotator const Rotation(0.0f, TurnYaw + Setting.YawOffset, 0.0f);

    FVector const Scale = Style.GlobalScale * Setting.Scale * TileScale;

    return FTransform(Rotation, TileCentre + Offset, Scale);
}
}  // namespace

UPacWallBuilderComponent::UPacWallBuilderComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPacWallBuilderComponent::BuildWalls(FPacGrid const & Grid, float const TileSize)
{
    ClearWalls();

    if (Style == nullptr)
    {
        UE_LOG(LogPacMan,
               Log,
               TEXT("PAC: WallBuilder has no Style: no wall meshes are "
                    "built."));
        return;
    }

    FIntPoint const Size = Grid.GetSize();

    int32 WallCount  = 0;
    int32 Unresolved = 0;

    // 1. Decide the piece and transform of every wall tile (cheap, no loading).
    for (int32 Y = 0; Y < Size.Y; ++Y)
    {
        for (int32 X = 0; X < Size.X; ++X)
        {
            FPacGridPosition const Position(X, Y);
            FPacWallPlacement      Placement;

            if (!Grid.TryGetWallPlacement(Position, bOutsideIsWall, Placement))
            {
                continue;  // not a wall
            }

            ++WallCount;

            FPacWallPieceSetting Setting;

            if (!Style->TryResolve(Placement.Piece, Setting))
            {
                ++Unresolved;
                continue;
            }

            PendingInstances.FindOrAdd(Setting.Mesh.ToSoftObjectPath())
                .Add(MakeWallTransform(Position, Placement, Setting, *Style, TileSize));
        }
    }

    if (Unresolved > 0)
    {
        UE_LOG(LogPacMan,
               Warning,
               TEXT("PAC: %d of %d wall tiles have no mesh (set a mesh for the "
                    "piece "
                    "or the style's DefaultPiece)."),
               Unresolved,
               WallCount);
    }

    if (PendingInstances.IsEmpty())
    {
        return;
    }

    // 2. Load only the meshes that are needed, in the background.
    TArray<FSoftObjectPath> Paths;
    PendingInstances.GetKeys(Paths);

    LoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        Paths,
        FStreamableDelegate::CreateUObject(this,
                                           &UPacWallBuilderComponent::HandleMeshesLoaded));
}

void UPacWallBuilderComponent::HandleMeshesLoaded()
{
    AActor* const Owner = GetOwner();

    if (Owner == nullptr)
    {
        return;
    }

    int32 InstanceCount = 0;

    // 3. One instanced mesh component per mesh.
    for (TPair<FSoftObjectPath, TArray<FTransform>> const & Pair : PendingInstances)
    {
        UStaticMesh* const Mesh = Cast<UStaticMesh>(Pair.Key.ResolveObject());

        if (Mesh == nullptr)
        {
            UE_LOG(LogPacMan,
                   Error,
                   TEXT("PAC: Wall mesh '%s' could not be loaded: %d tiles "
                        "left empty."),
                   *Pair.Key.ToString(),
                   Pair.Value.Num());
            continue;
        }

        UInstancedStaticMeshComponent* const
            Instances = NewObject<UInstancedStaticMeshComponent>(Owner);

        Instances->SetStaticMesh(Mesh);
        Instances->SetCollisionEnabled(
            ECollisionEnabled::NoCollision);  // movement uses the grid
        Instances->SetCastShadow(Style != nullptr && Style->bCastShadow);
        Instances->SetupAttachment(this);
        Instances->RegisterComponent();

        Instances->AddInstances(Pair.Value, false, false);

        WallComponents.Add(Instances);
        InstanceCount += Pair.Value.Num();
    }

    PendingInstances.Reset();

    UE_LOG(LogPacMan,
           Log,
           TEXT("PAC: Walls built: %d instances in %d mesh component(s)."),
           InstanceCount,
           WallComponents.Num());
}

void UPacWallBuilderComponent::ClearWalls()
{
    // Dropping the handle cancels a load that is still running.
    LoadHandle.Reset();
    PendingInstances.Reset();

    for (UInstancedStaticMeshComponent* Instances : WallComponents)
    {
        if (IsValid(Instances))
        {
            Instances->DestroyComponent();
        }
    }

    WallComponents.Reset();
}
