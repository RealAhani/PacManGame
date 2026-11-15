#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "PacGridTypes.generated.h"
// clang-format on

UENUM(BlueprintType)
enum class EPacDirection : uint8
{
    None,
    Up,
    Down,
    Left,
    Right
};

UENUM(BlueprintType)
enum class EPacTileType : uint8
{
    Empty,
    Wall,
    Pellet,
    PowerPellet,
    // Inside the ghost house. Only ghosts that are leaving it, waiting in it,
    // or returning to it after being eaten may walk here. Never Pac-Man.
    GhostHouse
};

// The wall mesh pieces. Every wall tile looks at its 8 neighbours and becomes
// one of these, turned by 0..3 quarter turns (see FPacGrid::TryGetWallPlacement).
//
// Each piece is described in its BASE orientation, as seen from above with
// North = +Y (Y grows with the row index of the layout text) and East = +X. '#'
// = that neighbour is a wall, '.' = it is not.
//
//   Alone          End            Corner         CornerFilled   Straight
//   . . .          . # .          . # .          . # #          . # .
//   . X .          . X .          . X #          . X #          . X .
//   . . .          . . .          . . .          . . .          . # .
//
//   TeeOpen        TeeFillNE      TeeFillSE      TeeFilled      CrossOpen
//   . # .          . # #          . # .          . # #          . # .
//   . X #          . X #          . X #          . X #          # X #
//   . # .          . # .          . # #          . # #          . # .
//
//   CrossFill1     CrossFill2Adj  CrossFill2Opp  CrossFill3     Solid
//   . # #          . # #          . # #          . # #          # # #
//   # X #          # X #          # X #          # X #          # X #
//   . # .          . # #          # # .          # # #          # # #
//
// A diagonal neighbour only counts when both cardinal neighbours next to it are
// walls too (otherwise it cannot change how the tile looks).
UENUM(BlueprintType)
enum class EPacWallPiece : uint8
{
    Alone,
    End,
    Corner,
    CornerFilled,
    Straight,
    TeeOpen,
    TeeFillNE,
    TeeFillSE,
    TeeFilled,
    CrossOpen,
    CrossFill1,
    CrossFill2Adjacent,
    CrossFill2Opposite,
    CrossFill3,
    Solid
};


USTRUCT(BlueprintType)
struct PACMANGAME_API FPacGridPosition
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 X = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 Y = 0;

    FPacGridPosition() = default;

    FPacGridPosition(int32 const InX, int32 const InY) : X(InX), Y(InY)
    {
    }

    bool operator==(FPacGridPosition const & Other) const
    {
        return X == Other.X && Y == Other.Y;
    }

    bool operator!=(FPacGridPosition const & Other) const
    {
        return !(*this == Other);
    }
};

FORCEINLINE uint32 GetTypeHash(FPacGridPosition const & Position)
{
    return HashCombine(::GetTypeHash(Position.X), ::GetTypeHash(Position.Y));
}

FORCEINLINE FPacGridPosition GetGridOffset(EPacDirection Direction)
{
    switch (Direction)
    {
        case EPacDirection::Up:
            return FPacGridPosition(0, 1);

        case EPacDirection::Down:
            return FPacGridPosition(0, -1);

        case EPacDirection::Left:
            return FPacGridPosition(-1, 0);

        case EPacDirection::Right:
            return FPacGridPosition(1, 0);

        case EPacDirection::None:
        default:
            return FPacGridPosition(0, 0);
    }
}

FORCEINLINE FPacGridPosition GetNeighbor(FPacGridPosition const & Position,
                                         EPacDirection            Direction)
{
    FPacGridPosition const Offset = GetGridOffset(Direction);

    return FPacGridPosition(Position.X + Offset.X, Position.Y + Offset.Y);
}
