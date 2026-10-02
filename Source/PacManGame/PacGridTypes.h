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
    PowerPellet
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