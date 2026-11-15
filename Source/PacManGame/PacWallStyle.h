#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "PacGridTypes.h"
#include "PacWallStyle.generated.h"
// clang-format on

// How one wall piece looks and sits.
//
// Model every mesh in its BASE orientation (see the drawing in PacGridTypes.h):
// seen from above, the side it connects to is North = +Y, and it fills one tile
// (100 x 100 units, or the Reference Tile Size of the style), centred on its
// pivot. If a mesh was made facing another way, fix it with Yaw Offset instead
// of re-exporting it.
USTRUCT(BlueprintType)
struct PACMANGAME_API FPacWallPieceSetting
{
    GENERATED_BODY()

    // Soft reference: the mesh is only loaded when the maze uses this piece.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
    TSoftObjectPtr<UStaticMesh> Mesh;

    // Extra size for this piece only (multiplied with the style's Global Scale).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
    FVector Scale = FVector::OneVector;

    // Moves the mesh inside its tile, in the piece's base orientation (it turns
    // together with the piece). Use Z to lift it off the floor.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
    FVector Offset = FVector::ZeroVector;

    // Extra turn in degrees, added to the piece's own rotation. For a mesh
    // whose connection is not along +Y: a mesh modelled along +X needs 90.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
    float YawOffset = 0.0f;
};

// The look of all walls of a maze: one mesh setting per wall piece.
// Create it in the Content Browser (Miscellaneous > Data Asset > PacWallStyle)
// and assign it to the Wall Builder component of the maze.
//
// Thick wall blocks (the classic maze) mostly need these five pieces:
//   TeeFilled   a flat edge of a block      CornerFilled  an outer corner
//   CrossFill3  an inner (concave) corner   Solid         the inside of a block
//   Straight    a thin one-tile wall
// The rest (End, Corner, Alone, TeeOpen, CrossOpen, ...) are for thin pipe-like
// walls and can be left empty: they fall back to a similar piece or DefaultPiece.
UCLASS(BlueprintType)
class PACMANGAME_API UPacWallStyle : public UDataAsset
{
    GENERATED_BODY()

public:

    // The piece to use for a tile. A piece with no mesh falls back to the
    // plainer piece with the same side neighbours (CornerFilled -> Corner,
    // ...), and finally to DefaultPiece. False if nothing at all has a mesh.
    bool TryResolve(EPacWallPiece Piece, FPacWallPieceSetting & OutSetting) const;

    // One entry per piece. Press "Add Missing Pieces" to list all of them.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Walls")
    TMap<EPacWallPiece, FPacWallPieceSetting> Pieces;

    // Used for any piece that has no mesh of its own (for example a plain cube).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Walls")
    FPacWallPieceSetting DefaultPiece;

    // Multiplied into every piece's Scale.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Walls")
    FVector GlobalScale = FVector::OneVector;

    // The tile size (in units) your meshes were made for. If the maze uses a
    // different tile size, every mesh is scaled by TileSize / this.
    UPROPERTY(EditAnywhere,
              BlueprintReadOnly,
              Category = "Walls",
              meta     = (ClampMin = "1.0"))
    float ReferenceTileSize = 100.0f;

    // Dynamic shadows from walls cost a lot on phones.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Walls")
    bool bCastShadow = false;

#if WITH_EDITOR
    // Adds an empty entry for every piece so they are all visible in the list.
    UFUNCTION(CallInEditor, Category = "Walls")
    void AddMissingPieces();
#endif
};
