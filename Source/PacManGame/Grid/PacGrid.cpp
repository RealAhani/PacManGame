// clang-format off
#include "PacGrid.h"
// clang-format on

namespace
{
// Maps any integer onto 0..Count-1 (also for negative values).
int32 WrapIndex(int32 const Value, int32 const Count)
{
    return ((Value % Count) + Count) % Count;
}

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
        case TEXT('H'):
            OutTile = EPacTileType::GhostHouse;
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

bool FPacGrid::IsWalkable(FPacGridPosition const & Position,
                          bool const               bAllowGhostHouse) const
{
    if (!IsValidPosition(Position))
    {
        return false;
    }

    EPacTileType const Tile = Tiles[GetIndex(Position)];

    if (Tile == EPacTileType::Wall)
    {
        return false;
    }

    return bAllowGhostHouse || Tile != EPacTileType::GhostHouse;
}

bool FPacGrid::IsGhostHouse(FPacGridPosition const & Position) const
{
    return IsValidPosition(Position) &&
           Tiles[GetIndex(Position)] == EPacTileType::GhostHouse;
}

bool FPacGrid::IsWrapStep(FPacGridPosition const & From, FPacGridPosition const & To)
{
    return FMath::Abs(From.X - To.X) > 1 || FMath::Abs(From.Y - To.Y) > 1;
}

bool FPacGrid::TryGetNeighborWrapping(FPacGridPosition const & Position,
                                      EPacDirection const      Direction,
                                      FPacGridPosition & OutNeighbor) const
{
    if (TryGetNeighbor(Position, Direction, OutNeighbor))
    {
        return true;
    }

    // Off the edge: only an open tile may leave, and only onto an open tile.
    if (Size.X <= 0 || Size.Y <= 0 || !IsWalkable(Position))
    {
        return false;
    }

    FPacGridPosition const Outside = GetNeighbor(Position, Direction);
    FPacGridPosition const Wrapped(WrapIndex(Outside.X, Size.X),
                                   WrapIndex(Outside.Y, Size.Y));

    if (!IsWalkable(Wrapped))
    {
        return false;
    }

    OutNeighbor = Wrapped;
    return true;
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
                             EPacDirection &          OutFirstStep,
                             bool const               bAllowGhostHouse) const
{
    OutFirstStep = EPacDirection::None;

    if (!IsWalkable(Start, bAllowGhostHouse) ||
        !IsWalkable(Target, bAllowGhostHouse) || Start == Target)
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

            if (!TryGetNeighborWrapping(Current, Direction, Next) ||
                !IsWalkable(Next, bAllowGhostHouse))
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

bool FPacGrid::BuildDistanceField(FPacGridPosition const & Origin,
                                  TArray<int32> &          OutDistances,
                                  bool const bAllowGhostHouse) const
{
    int32 const CellCount = Size.X * Size.Y;

    OutDistances.Init(INDEX_NONE, CellCount);

    if (!IsWalkable(Origin, bAllowGhostHouse))
    {
        return false;
    }

    TArray<FPacGridPosition> Queue;
    Queue.Reserve(CellCount);

    Queue.Add(Origin);
    OutDistances[GetIndex(Origin)] = 0;

    for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
    {
        FPacGridPosition const Current = Queue[QueueIndex];
        int32 const CurrentDistance    = OutDistances[GetIndex(Current)];

        for (EPacDirection const Direction : AllDirections)
        {
            FPacGridPosition Next;

            if (!TryGetNeighborWrapping(Current, Direction, Next) ||
                !IsWalkable(Next, bAllowGhostHouse))
            {
                continue;
            }

            int32 const NextIndex = GetIndex(Next);

            if (OutDistances[NextIndex] != INDEX_NONE)
            {
                continue;
            }

            OutDistances[NextIndex] = CurrentDistance + 1;
            Queue.Add(Next);
        }
    }

    return true;
}

int32 FPacGrid::GetFieldDistance(TArray<int32> const &    Field,
                                 FPacGridPosition const & Position) const
{
    if (!IsValidPosition(Position) || Field.Num() != Size.X * Size.Y)
    {
        return INDEX_NONE;
    }

    return Field[GetIndex(Position)];
}

bool FPacGrid::FindFleeTarget(FPacGridPosition const & Fleer,
                              FPacGridPosition const & Threat,
                              FPacGridPosition &       OutTarget,
                              bool const               bAllowGhostHouse) const
{
    TArray<int32> FleerField;
    TArray<int32> ThreatField;

    if (!BuildDistanceField(Fleer, FleerField, bAllowGhostHouse) ||
        !BuildDistanceField(Threat, ThreatField, bAllowGhostHouse))
    {
        return false;
    }

    // A tile the Threat cannot reach at all is as safe as it gets.
    int32 const Unreachable = Size.X * Size.Y;

    bool  bFound     = false;
    int32 BestThreat = 0;

    for (int32 Y = 0; Y < Size.Y; ++Y)
    {
        for (int32 X = 0; X < Size.X; ++X)
        {
            FPacGridPosition const Candidate(X, Y);
            int32 const            Index = GetIndex(Candidate);

            int32 const FleerDistance = FleerField[Index];

            // Unreachable for the Fleer, or the Fleer's own tile.
            if (FleerDistance <= 0)
            {
                continue;
            }

            int32 const ThreatDistance = ThreatField[Index] == INDEX_NONE
                                             ? Unreachable
                                             : ThreatField[Index];

            // The Threat gets there first, or at the same time.
            if (FleerDistance >= ThreatDistance)
            {
                continue;
            }

            if (!bFound || ThreatDistance > BestThreat)
            {
                bFound     = true;
                BestThreat = ThreatDistance;
                OutTarget  = Candidate;
            }
        }
    }

    return bFound;
}


bool FPacGrid::FindNearestWalkable(FPacGridPosition const & Desired,
                                   FPacGridPosition &       OutPosition,
                                   bool const bAllowGhostHouse) const
{
    if (IsWalkable(Desired, bAllowGhostHouse))
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

            if (!IsWalkable(Candidate, bAllowGhostHouse))
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
