// clang-format off
#include "PacGrid.h"
// clang-format on

namespace
{
// Order matters: it is the tie-break order of FindFirstStep.
constexpr EPacDirection AllDirections[] = {EPacDirection::Up,
                                           EPacDirection::Down,
                                           EPacDirection::Left,
                                           EPacDirection::Right};

bool TryParseTileCharacter(TCHAR const Character, EPacTileType & OutTile)
{
    switch (Character)
    {
        case TEXT('#'):
            OutTile = EPacTileType::Wall;
            return true;

        case TEXT('.'):
            OutTile = EPacTileType::Pellet;
            return true;

        case TEXT('-'):
            OutTile = EPacTileType::Empty;
            return true;

        case TEXT('o'):
            OutTile = EPacTileType::PowerPellet;
            return true;

        default:
            return false;
    }
}
}  // namespace

// ---------------------------------------------------------------------------
// building
// ---------------------------------------------------------------------------

bool FPacGrid::TryParse(FString const & Layout,
                        FIntPoint const ExpectedSize,
                        FPacGrid &      OutGrid,
                        FString &       OutError)
{
    OutGrid = FPacGrid();
    OutError.Reset();

    if (ExpectedSize.X <= 0 || ExpectedSize.Y <= 0)
    {
        OutError = FString::Printf(TEXT("Invalid grid size: %d x %d."),
                                   ExpectedSize.X,
                                   ExpectedSize.Y);
        return false;
    }

    FString Normalized = Layout;
    Normalized.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
    Normalized.ReplaceInline(TEXT("\r"), TEXT("\n"));
    Normalized.TrimEndInline();

    TArray<FString> Rows;
    Normalized.ParseIntoArray(Rows, TEXT("\n"), false);

    if (Rows.IsEmpty())
    {
        OutError = TEXT("The layout is empty.");
        return false;
    }

    if (Rows.Num() != ExpectedSize.Y)
    {
        OutError = FString::Printf(TEXT("The layout has %d rows, but %d are "
                                        "expected."),
                                   Rows.Num(),
                                   ExpectedSize.Y);
        return false;
    }

    FPacGrid Result;
    Result.Size = ExpectedSize;
    Result.Tiles.Init(EPacTileType::Empty, ExpectedSize.X * ExpectedSize.Y);

    for (int32 Y = 0; Y < ExpectedSize.Y; ++Y)
    {
        if (Rows[Y].Len() != ExpectedSize.X)
        {
            OutError = FString::Printf(TEXT("Row %d has %d columns, but %d are "
                                            "expected."),
                                       Y,
                                       Rows[Y].Len(),
                                       ExpectedSize.X);
            return false;
        }

        for (int32 X = 0; X < ExpectedSize.X; ++X)
        {
            EPacTileType Tile = EPacTileType::Empty;

            if (!TryParseTileCharacter(Rows[Y][X], Tile))
            {
                OutError = FString::Printf(TEXT("Invalid character '%c' at "
                                                "(%d, %d)."),
                                           Rows[Y][X],
                                           X,
                                           Y);
                return false;
            }

            Result.Tiles[Y * ExpectedSize.X + X] = Tile;
        }
    }

    OutGrid = MoveTemp(Result);
    return true;
}

// ---------------------------------------------------------------------------
// queries
// ---------------------------------------------------------------------------

int32 FPacGrid::GetIndex(FPacGridPosition const & Position) const
{
    check(IsValidPosition(Position));

    return Position.Y * Size.X + Position.X;
}

bool FPacGrid::IsValidPosition(FPacGridPosition const & Position) const
{
    return Position.X >= 0 && Position.X < Size.X && Position.Y >= 0 &&
           Position.Y < Size.Y;
}

EPacTileType FPacGrid::GetTile(FPacGridPosition const & Position) const
{
    return Tiles[GetIndex(Position)];
}

bool FPacGrid::TryGetTile(FPacGridPosition const & Position,
                          EPacTileType &           OutTile) const
{
    if (!IsValidPosition(Position))
    {
        return false;
    }

    OutTile = Tiles[GetIndex(Position)];
    return true;
}

bool FPacGrid::IsWalkable(FPacGridPosition const & Position) const
{
    if (!IsValidPosition(Position))
    {
        return false;
    }

    return Tiles[GetIndex(Position)] != EPacTileType::Wall;
}

bool FPacGrid::TryGetNeighbor(FPacGridPosition const & Position,
                              EPacDirection const      Direction,
                              FPacGridPosition &       OutNeighbor) const
{
    FPacGridPosition const Neighbor = GetNeighbor(Position, Direction);

    if (!IsValidPosition(Neighbor))
    {
        return false;
    }

    OutNeighbor = Neighbor;
    return true;
}

int32 FPacGrid::CountTiles(EPacTileType const TileType) const
{
    int32 Count = 0;

    for (EPacTileType const Tile : Tiles)
    {
        if (Tile == TileType)
        {
            ++Count;
        }
    }

    return Count;
}

// ---------------------------------------------------------------------------
// changing
// ---------------------------------------------------------------------------

void FPacGrid::SetTile(FPacGridPosition const & Position, EPacTileType const TileType)
{
    Tiles[GetIndex(Position)] = TileType;
}

// ---------------------------------------------------------------------------
// path finding
// ---------------------------------------------------------------------------

bool FPacGrid::FindFirstStep(FPacGridPosition const & Start,
                             FPacGridPosition const & Target,
                             EPacDirection const      ForbiddenFirstStep,
                             EPacDirection &          OutFirstStep) const
{
    OutFirstStep = EPacDirection::None;

    if (!IsWalkable(Start) || !IsWalkable(Target) || Start == Target)
    {
        return false;
    }

    int32 const CellCount = Size.X * Size.Y;

    // Flat arrays indexed by cell: no per-step TSet/TMap allocations.
    TArray<bool>          Visited;
    TArray<EPacDirection> FirstStepOf;
    Visited.Init(false, CellCount);
    FirstStepOf.Init(EPacDirection::None, CellCount);

    TArray<FPacGridPosition> Queue;
    Queue.Reserve(CellCount);

    Queue.Add(Start);
    Visited[GetIndex(Start)] = true;

    for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
    {
        FPacGridPosition const Current  = Queue[QueueIndex];
        bool const             bIsStart = QueueIndex == 0;

        for (EPacDirection const Direction : AllDirections)
        {
            // The restriction only applies to the very first step.
            if (bIsStart && Direction == ForbiddenFirstStep)
            {
                continue;
            }

            FPacGridPosition Next;

            if (!TryGetNeighbor(Current, Direction, Next) || !IsWalkable(Next))
            {
                continue;
            }

            int32 const NextIndex = GetIndex(Next);

            if (Visited[NextIndex])
            {
                continue;
            }

            Visited[NextIndex] = true;

            EPacDirection const FirstStep = bIsStart
                                                ? Direction
                                                : FirstStepOf[GetIndex(Current)];

            FirstStepOf[NextIndex] = FirstStep;

            if (Next == Target)
            {
                OutFirstStep = FirstStep;
                return true;
            }

            Queue.Add(Next);
        }
    }

    return false;
}

bool FPacGrid::FindNearestWalkable(FPacGridPosition const & Desired,
                                   FPacGridPosition &       OutPosition) const
{
    if (IsWalkable(Desired))
    {
        OutPosition = Desired;
        return true;
    }

    int32 BestDistance = MAX_int32;
    bool  bFound       = false;

    for (int32 Y = 0; Y < Size.Y; ++Y)
    {
        for (int32 X = 0; X < Size.X; ++X)
        {
            FPacGridPosition const Candidate(X, Y);

            if (!IsWalkable(Candidate))
            {
                continue;
            }

            int32 const Distance = FMath::Abs(X - Desired.X) +
                                   FMath::Abs(Y - Desired.Y);

            if (Distance < BestDistance)
            {
                BestDistance = Distance;
                OutPosition  = Candidate;
                bFound       = true;
            }
        }
    }

    return bFound;
}
