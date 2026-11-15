// clang-format off
#include "PacWallStyle.h"
#include "Grid/PacGrid.h"
// clang-format on

bool UPacWallStyle::TryResolve(EPacWallPiece const    Piece,
                               FPacWallPieceSetting & OutSetting) const
{
    EPacWallPiece Current = Piece;

    // The chain is at most 2 long; the counter only protects against a loop if
    // the fallback rules are ever edited wrongly.
    for (int32 Guard = 0; Guard < 8; ++Guard)
    {
        FPacWallPieceSetting const* Found = Pieces.Find(Current);

        if (Found != nullptr && !Found->Mesh.IsNull())
        {
            OutSetting = *Found;
            return true;
        }

        EPacWallPiece Next;

        if (!FPacGrid::TryGetWallPieceFallback(Current, Next))
        {
            break;
        }

        Current = Next;
    }

    if (!DefaultPiece.Mesh.IsNull())
    {
        OutSetting = DefaultPiece;
        return true;
    }

    return false;
}

#if WITH_EDITOR
void UPacWallStyle::AddMissingPieces()
{
    Modify();

    UEnum const* const PieceEnum = StaticEnum<EPacWallPiece>();

    // The last entry of a UENUM is the hidden _MAX value: skip it.
    for (int32 Index = 0; Index < PieceEnum->NumEnums() - 1; ++Index)
    {
        Pieces.FindOrAdd(
            static_cast<EPacWallPiece>(PieceEnum->GetValueByIndex(Index)));
    }
}
#endif
