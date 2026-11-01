#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "..\PacGridTypes.h"
// clang-format on

/**
 * The maze as plain data: tiles plus the questions you can ask about them.
 *
 * This is NOT a UObject, Actor or USTRUCT. It needs no World, so it can be
 * built from a string and unit tested on its own (see Tests/PacGridSpec.cpp).
 * APacMaze owns one and only adds the Unreal side (world positions, camera,
 * debug drawing).
 *
 * Coordinates: X grows to the right, Y grows with the row index in the layout
 * text (row 0 of the text is Y = 0). EPacDirection::Up means Y + 1.
 */
struct PACMANGAME_API FPacGrid
{
public:

    // -------------------- building --------------------

    // Parses a text layout. '#' wall, '.' pellet, 'o' power pellet, '-' empty.
    // Line endings may be \n or \r\n, trailing blank lines are ignored.
    // On failure returns false, leaves OutGrid empty and fills OutError.
    static bool TryParse(FString const & Layout,
                         FIntPoint       ExpectedSize,
                         FPacGrid &      OutGrid,
                         FString &       OutError);

    // -------------------- queries --------------------

    FIntPoint GetSize() const
    {
        return Size;
    }

    bool         IsValidPosition(FPacGridPosition const & Position) const;
    EPacTileType GetTile(FPacGridPosition const & Position) const;
    bool TryGetTile(FPacGridPosition const & Position, EPacTileType & OutTile) const;
    bool IsWalkable(FPacGridPosition const & Position) const;
    bool TryGetNeighbor(FPacGridPosition const & Position,
                        EPacDirection            Direction,
                        FPacGridPosition &       OutNeighbor) const;

    // how many tile of the specified tiles exist in the map
    int32 CountTiles(EPacTileType TileType) const;

    // -------------------- changing --------------------

    void SetTile(FPacGridPosition const & Position, EPacTileType TileType);

    // -------------------- path finding --------------------

    // Breadth-first search from Start to Target over walkable tiles.
    // Returns the FIRST STEP of a shortest path in OutFirstStep.
    //
    // ForbiddenFirstStep: this direction may not be used for the first step
    // (pass the opposite of the travel direction to forbid reversing, or
    // EPacDirection::None to allow everything). Later steps are unrestricted.
    //
    // Ties are broken in the order Up, Down, Left, Right.
    // Returns false if Start or Target is not walkable, Start == Target, or
    // no route exists. OutFirstStep is None in that case.
    bool FindFirstStep(FPacGridPosition const & Start,
                       FPacGridPosition const & Target,
                       EPacDirection            ForbiddenFirstStep,
                       EPacDirection &          OutFirstStep) const;

    // Finds the walkable tile closest to Desired (Manhattan distance). Desired
    // may be inside a wall or outside the grid. Returns the walkable tile
    // itself if Desired is already walkable. False only if nothing is walkable.
    bool FindNearestWalkable(FPacGridPosition const & Desired,
                             FPacGridPosition &       OutPosition) const;

private:

    int32 GetIndex(FPacGridPosition const & Position) const;

    FIntPoint            Size = FIntPoint::ZeroValue;
    TArray<EPacTileType> Tiles;
};
