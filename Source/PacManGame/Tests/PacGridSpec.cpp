// Unit tests for FPacGrid. No world, no actors: just text in, answers out.
//
// Run in the editor: Tools > Session Frontend > Automation > "PacManGame.Grid"
// Or from a shell:
//   UnrealEditor-Cmd.exe <Project>.uproject -ExecCmds="Automation RunTests
//   PacManGame.Grid; Quit" -unattended -nullrhi -log

// clang-format off
#include "Misc/AutomationTest.h"
#include "Grid/PacGrid.h"
// clang-format on

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
// Layout text: '#' wall, '.' pellet, 'o' power pellet, '-' empty.
// Row 0 of the text is Y = 0, and Up means Y + 1.

// 5x3, one straight corridor.
constexpr TCHAR const* CorridorLayout = TEXT(
    "#####\n"
    "#---#\n"
    "#####");
FIntPoint const CorridorSize(5, 3);

// 5x5, a ring of floor around a single wall block at (2,2).
constexpr TCHAR const* RingLayout = TEXT(
    "#####\n"
    "#---#\n"
    "#-#-#\n"
    "#---#\n"
    "#####");
FIntPoint const RingSize(5, 5);

// 5x3, two rooms that are not connected.
constexpr TCHAR const* SplitLayout = TEXT(
    "#####\n"
    "#-#-#\n"
    "#####");
FIntPoint const SplitSize(5, 3);

FPacGrid MakeGrid(FString const & Layout, FIntPoint const Size)
{
    FPacGrid Grid;
    FString  Error;

    bool const bParsed = FPacGrid::TryParse(Layout, Size, Grid, Error);

    // Only used with layouts written in this file, so a failure here is a
    // typo in the test itself.
    checkf(bParsed, TEXT("Test layout is invalid: %s"), *Error);

    return Grid;
}
}  // namespace

BEGIN_DEFINE_SPEC(FPacGridSpec,
                  "PacManGame.Grid",
                  EAutomationTestFlags::EditorContext |
                      EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FPacGridSpec)

void FPacGridSpec::Define()
{
    // ------------------------------------------------------------------
    Describe("TryParse",
             [this]()
             {
                 It("builds the grid from a valid layout",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(TEXT("#####\n#.o-#\n#####"),
                                                 FIntPoint(5, 3));

                        TestTrue(TEXT("size"), Grid.GetSize() == FIntPoint(5, 3));
                        TestTrue(TEXT("wall"),
                                 Grid.GetTile(FPacGridPosition(0, 0)) ==
                                     EPacTileType::Wall);
                        TestTrue(TEXT("pellet"),
                                 Grid.GetTile(FPacGridPosition(1, 1)) ==
                                     EPacTileType::Pellet);
                        TestTrue(TEXT("power pellet"),
                                 Grid.GetTile(FPacGridPosition(2, 1)) ==
                                     EPacTileType::PowerPellet);
                        TestTrue(TEXT("empty"),
                                 Grid.GetTile(FPacGridPosition(3, 1)) ==
                                     EPacTileType::Empty);
                        TestEqual(TEXT("wall count"),
                                  Grid.CountTiles(EPacTileType::Wall),
                                  12);
                        TestEqual(TEXT("pellet count"),
                                  Grid.CountTiles(EPacTileType::Pellet),
                                  1);
                    });

                 It("accepts Windows line endings and trailing blank lines",
                    [this]()
                    {
                        FPacGrid Grid;
                        FString  Error;

                        bool const bParsed = FPacGrid::TryParse(TEXT("#####"
                                                                     "\r\n#...#"
                                                                     "\r\n#####"
                                                                     "\r\n\r"
                                                                     "\n"),
                                                                FIntPoint(5, 3),
                                                                Grid,
                                                                Error);

                        TestTrue(TEXT("parsed"), bParsed);
                        TestTrue(TEXT("size"), Grid.GetSize() == FIntPoint(5, 3));
                    });

                 It("rejects the wrong number of rows",
                    [this]()
                    {
                        FPacGrid Grid;
                        FString  Error;

                        bool const bParsed = FPacGrid::TryParse(TEXT("#####\n#."
                                                                     "..#"),
                                                                FIntPoint(5, 3),
                                                                Grid,
                                                                Error);

                        TestFalse(TEXT("parsed"), bParsed);
                        TestFalse(TEXT("has an error message"), Error.IsEmpty());
                        TestTrue(TEXT("grid stays empty"),
                                 Grid.GetSize() == FIntPoint::ZeroValue);
                    });

                 It("rejects a row with the wrong number of columns",
                    [this]()
                    {
                        FPacGrid Grid;
                        FString  Error;

                        bool const bParsed = FPacGrid::TryParse(TEXT("#####\n#."
                                                                     ".#\n####"
                                                                     "#"),
                                                                FIntPoint(5, 3),
                                                                Grid,
                                                                Error);

                        TestFalse(TEXT("parsed"), bParsed);
                        TestTrue(TEXT("names the row"),
                                 Error.Contains(TEXT("Row 1")));
                    });

                 It("rejects an invalid character and says where it is",
                    [this]()
                    {
                        FPacGrid Grid;
                        FString  Error;

                        bool const bParsed = FPacGrid::TryParse(TEXT("#####\n#."
                                                                     "x.#\n####"
                                                                     "#"),
                                                                FIntPoint(5, 3),
                                                                Grid,
                                                                Error);

                        TestFalse(TEXT("parsed"), bParsed);
                        TestTrue(TEXT("names the position"),
                                 Error.Contains(TEXT("(2, 1)")));
                    });

                 It("rejects an empty layout",
                    [this]()
                    {
                        FPacGrid Grid;
                        FString  Error;

                        TestFalse(TEXT("parsed"),
                                  FPacGrid::TryParse(FString(),
                                                     FIntPoint(5, 3),
                                                     Grid,
                                                     Error));
                        TestFalse(TEXT("has an error message"), Error.IsEmpty());
                    });

                 It("rejects a size that is not positive",
                    [this]()
                    {
                        FPacGrid Grid;
                        FString  Error;

                        TestFalse(TEXT("parsed"),
                                  FPacGrid::TryParse(TEXT("#####"),
                                                     FIntPoint(0, 1),
                                                     Grid,
                                                     Error));
                        TestFalse(TEXT("has an error message"), Error.IsEmpty());
                    });
             });

    // ------------------------------------------------------------------
    Describe("Queries",
             [this]()
             {
                 It("reports walkability and treats out-of-range as not "
                    "walkable",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(TEXT("#####\n#.o-#\n#####"),
                                                 FIntPoint(5, 3));

                        TestFalse(TEXT("wall"),
                                  Grid.IsWalkable(FPacGridPosition(0, 0)));
                        TestTrue(TEXT("pellet"),
                                 Grid.IsWalkable(FPacGridPosition(1, 1)));
                        TestTrue(TEXT("power pellet"),
                                 Grid.IsWalkable(FPacGridPosition(2, 1)));
                        TestTrue(TEXT("empty"),
                                 Grid.IsWalkable(FPacGridPosition(3, 1)));
                        TestFalse(TEXT("left of grid"),
                                  Grid.IsWalkable(FPacGridPosition(-1, 1)));
                        TestFalse(TEXT("right of grid"),
                                  Grid.IsWalkable(FPacGridPosition(5, 1)));
                        TestFalse(TEXT("below grid"),
                                  Grid.IsWalkable(FPacGridPosition(1, -1)));
                        TestFalse(TEXT("above grid"),
                                  Grid.IsWalkable(FPacGridPosition(1, 3)));
                    });

                 It("finds neighbors inside the grid only, with Up meaning Y + "
                    "1",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(RingLayout, RingSize);

                        FPacGridPosition Neighbor;

                        TestTrue(TEXT("up from corner"),
                                 Grid.TryGetNeighbor(FPacGridPosition(0, 0),
                                                     EPacDirection::Up,
                                                     Neighbor));
                        TestTrue(TEXT("up is Y + 1"),
                                 Neighbor == FPacGridPosition(0, 1));

                        TestTrue(TEXT("right from corner"),
                                 Grid.TryGetNeighbor(FPacGridPosition(0, 0),
                                                     EPacDirection::Right,
                                                     Neighbor));
                        TestTrue(TEXT("right is X + 1"),
                                 Neighbor == FPacGridPosition(1, 0));

                        TestFalse(TEXT("left of the grid"),
                                  Grid.TryGetNeighbor(FPacGridPosition(0, 0),
                                                      EPacDirection::Left,
                                                      Neighbor));
                        TestFalse(TEXT("below the grid"),
                                  Grid.TryGetNeighbor(FPacGridPosition(0, 0),
                                                      EPacDirection::Down,
                                                      Neighbor));
                    });

                 It("changes walkability when a tile is set",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(CorridorLayout, CorridorSize);

                        TestTrue(TEXT("floor before"),
                                 Grid.IsWalkable(FPacGridPosition(2, 1)));

                        Grid.SetTile(FPacGridPosition(2, 1), EPacTileType::Wall);

                        TestFalse(TEXT("wall after"),
                                  Grid.IsWalkable(FPacGridPosition(2, 1)));
                    });
             });

    // ------------------------------------------------------------------
    Describe("FindNearestWalkable",
             [this]()
             {
                 It("returns the tile itself when it is already walkable",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(CorridorLayout, CorridorSize);

                        FPacGridPosition Result;

                        TestTrue(TEXT("found"),
                                 Grid.FindNearestWalkable(FPacGridPosition(2, 1),
                                                          Result));
                        TestTrue(TEXT("same tile"),
                                 Result == FPacGridPosition(2, 1));
                    });

                 It("moves a wall target to the closest floor",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(CorridorLayout, CorridorSize);

                        FPacGridPosition Result;

                        TestTrue(TEXT("found"),
                                 Grid.FindNearestWalkable(FPacGridPosition(2, 0),
                                                          Result));
                        TestTrue(TEXT("tile just above"),
                                 Result == FPacGridPosition(2, 1));
                    });

                 It("moves an off-grid target onto the grid",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(CorridorLayout, CorridorSize);

                        FPacGridPosition Result;

                        TestTrue(TEXT("found"),
                                 Grid.FindNearestWalkable(FPacGridPosition(20, 1),
                                                          Result));
                        TestTrue(TEXT("right end of the corridor"),
                                 Result == FPacGridPosition(3, 1));
                    });

                 It("fails when nothing is walkable",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(TEXT("###\n###"), FIntPoint(3, 2));

                        FPacGridPosition Result;

                        TestFalse(TEXT("found"),
                                  Grid.FindNearestWalkable(FPacGridPosition(1, 1),
                                                           Result));
                    });
             });

    // ------------------------------------------------------------------
    Describe("FindFirstStep",
             [this]()
             {
                 It("walks a straight corridor",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(CorridorLayout, CorridorSize);

                        EPacDirection Step = EPacDirection::None;

                        TestTrue(TEXT("route exists"),
                                 Grid.FindFirstStep(FPacGridPosition(1, 1),
                                                    FPacGridPosition(3, 1),
                                                    EPacDirection::None,
                                                    Step));
                        TestTrue(TEXT("goes right"), Step == EPacDirection::Right);

                        TestTrue(TEXT("route back exists"),
                                 Grid.FindFirstStep(FPacGridPosition(3, 1),
                                                    FPacGridPosition(1, 1),
                                                    EPacDirection::None,
                                                    Step));
                        TestTrue(TEXT("goes left"), Step == EPacDirection::Left);
                    });

                 It("returns false when start and target are the same",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(CorridorLayout, CorridorSize);

                        EPacDirection Step = EPacDirection::Up;

                        TestFalse(TEXT("no step needed"),
                                  Grid.FindFirstStep(FPacGridPosition(2, 1),
                                                     FPacGridPosition(2, 1),
                                                     EPacDirection::None,
                                                     Step));
                        TestTrue(TEXT("output reset to None"),
                                 Step == EPacDirection::None);
                    });

                 It("returns false when the target is a wall or off the grid",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(CorridorLayout, CorridorSize);

                        EPacDirection Step = EPacDirection::None;

                        TestFalse(TEXT("wall"),
                                  Grid.FindFirstStep(FPacGridPosition(1, 1),
                                                     FPacGridPosition(2, 0),
                                                     EPacDirection::None,
                                                     Step));
                        TestFalse(TEXT("off grid"),
                                  Grid.FindFirstStep(FPacGridPosition(1, 1),
                                                     FPacGridPosition(9, 1),
                                                     EPacDirection::None,
                                                     Step));
                    });

                 It("returns false when no route exists",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(SplitLayout, SplitSize);

                        EPacDirection Step = EPacDirection::None;

                        TestFalse(TEXT("separate rooms"),
                                  Grid.FindFirstStep(FPacGridPosition(1, 1),
                                                     FPacGridPosition(3, 1),
                                                     EPacDirection::None,
                                                     Step));
                    });

                 It("fails in a corridor when the only first step is forbidden",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(CorridorLayout, CorridorSize);

                        EPacDirection Step = EPacDirection::None;

                        TestFalse(TEXT("cannot go right"),
                                  Grid.FindFirstStep(FPacGridPosition(1, 1),
                                                     FPacGridPosition(3, 1),
                                                     EPacDirection::Right,
                                                     Step));
                    });

                 It("takes the shortest way round the ring",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(RingLayout, RingSize);

                        EPacDirection Step = EPacDirection::None;

                        TestTrue(TEXT("route exists"),
                                 Grid.FindFirstStep(FPacGridPosition(1, 1),
                                                    FPacGridPosition(3, 1),
                                                    EPacDirection::None,
                                                    Step));
                        TestTrue(TEXT("two steps right beats six steps around"),
                                 Step == EPacDirection::Right);
                    });

                 It("goes the long way round when the short first step is "
                    "forbidden",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(RingLayout, RingSize);

                        EPacDirection Step = EPacDirection::None;

                        TestTrue(TEXT("route exists"),
                                 Grid.FindFirstStep(FPacGridPosition(1, 1),
                                                    FPacGridPosition(3, 1),
                                                    EPacDirection::Right,
                                                    Step));
                        TestTrue(TEXT("forced to go up"), Step == EPacDirection::Up);
                    });

                 It("only restricts the first step, not later ones",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(RingLayout, RingSize);

                        EPacDirection Step = EPacDirection::None;

                        // Forbid Up. The route must start Right, then it is
                        // free to turn Up later on its way to (1,3).
                        TestTrue(TEXT("route exists"),
                                 Grid.FindFirstStep(FPacGridPosition(1, 1),
                                                    FPacGridPosition(1, 3),
                                                    EPacDirection::Up,
                                                    Step));
                        TestTrue(TEXT("starts right"), Step == EPacDirection::Right);
                    });

                 It("breaks ties in the order Up, Down, Left, Right",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(RingLayout, RingSize);

                        EPacDirection Step = EPacDirection::None;

                        // (3,3) is four steps away both ways round the ring.
                        TestTrue(TEXT("route exists"),
                                 Grid.FindFirstStep(FPacGridPosition(1, 1),
                                                    FPacGridPosition(3, 3),
                                                    EPacDirection::None,
                                                    Step));
                        TestTrue(TEXT("Up wins the tie"), Step == EPacDirection::Up);
                    });
             });

    // ------------------------------------------------------------------
    Describe("Tunnels",
             [this]()
             {
                 // The tunnel row is open at both ends of the grid.
                 constexpr TCHAR const* TunnelLayout = TEXT(
                     "#####\n"
                     "-----\n"
                     "#####");
                 FIntPoint const TunnelSize(5, 3);

                 It("wraps from the left edge to the right edge and back",
                    [this, TunnelLayout, TunnelSize]()
                    {
                        FPacGrid Grid = MakeGrid(TunnelLayout, TunnelSize);

                        FPacGridPosition Neighbor;

                        TestTrue(TEXT("left from the left edge"),
                                 Grid.TryGetNeighborWrapping(FPacGridPosition(0, 1),
                                                             EPacDirection::Left,
                                                             Neighbor));
                        TestTrue(TEXT("arrives at the right edge"),
                                 Neighbor == FPacGridPosition(4, 1));

                        TestTrue(TEXT("right from the right edge"),
                                 Grid.TryGetNeighborWrapping(FPacGridPosition(4, 1),
                                                             EPacDirection::Right,
                                                             Neighbor));
                        TestTrue(TEXT("arrives at the left edge"),
                                 Neighbor == FPacGridPosition(0, 1));
                    });

                 It("behaves like TryGetNeighbor away from the edge",
                    [this, TunnelLayout, TunnelSize]()
                    {
                        FPacGrid Grid = MakeGrid(TunnelLayout, TunnelSize);

                        FPacGridPosition Neighbor;

                        TestTrue(TEXT("inside the grid"),
                                 Grid.TryGetNeighborWrapping(FPacGridPosition(2, 1),
                                                             EPacDirection::Right,
                                                             Neighbor));
                        TestTrue(TEXT("plain next tile"),
                                 Neighbor == FPacGridPosition(3, 1));
                    });

                 It("never wraps in plain TryGetNeighbor",
                    [this, TunnelLayout, TunnelSize]()
                    {
                        FPacGrid Grid = MakeGrid(TunnelLayout, TunnelSize);

                        FPacGridPosition Neighbor;

                        TestFalse(TEXT("left of the grid"),
                                  Grid.TryGetNeighbor(FPacGridPosition(0, 1),
                                                      EPacDirection::Left,
                                                      Neighbor));
                    });

                 It("does not leave through a wall on the edge",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(RingLayout, RingSize);

                        FPacGridPosition Neighbor;

                        // (0,0) is a wall, so nothing leaves through it.
                        TestFalse(TEXT("wall corner"),
                                  Grid.TryGetNeighborWrapping(FPacGridPosition(0, 0),
                                                              EPacDirection::Left,
                                                              Neighbor));
                        // Inside the grid it is the plain neighbour (here a wall);
                        // like TryGetNeighbor, the caller checks IsWalkable.
                        TestTrue(TEXT("inside the grid"),
                                 Grid.TryGetNeighborWrapping(FPacGridPosition(1, 1),
                                                             EPacDirection::Left,
                                                             Neighbor));
                        TestFalse(TEXT("that neighbour is not walkable"),
                                  Grid.IsWalkable(Neighbor));
                    });

                 It("does not wrap onto a wall on the far side",
                    [this]()
                    {
                        // Open on the left, wall on the right.
                        FPacGrid Grid = MakeGrid(TEXT("#####\n"
                                                      "----#\n"
                                                      "#####"),
                                                 FIntPoint(5, 3));

                        FPacGridPosition Neighbor;

                        TestFalse(TEXT("blocked by the far wall"),
                                  Grid.TryGetNeighborWrapping(FPacGridPosition(0, 1),
                                                              EPacDirection::Left,
                                                              Neighbor));
                    });

                 It("wraps vertically too",
                    [this]()
                    {
                        FPacGrid Grid = MakeGrid(TEXT("#-#\n"
                                                      "#-#\n"
                                                      "#-#"),
                                                 FIntPoint(3, 3));

                        FPacGridPosition Neighbor;

                        TestTrue(TEXT("up from the top row"),
                                 Grid.TryGetNeighborWrapping(FPacGridPosition(1, 2),
                                                             EPacDirection::Up,
                                                             Neighbor));
                        TestTrue(TEXT("arrives at the bottom row"),
                                 Neighbor == FPacGridPosition(1, 0));
                    });

                 It("recognises a tunnel step",
                    [this]()
                    {
                        TestTrue(TEXT("edge to edge"),
                                 FPacGrid::IsWrapStep(FPacGridPosition(0, 1),
                                                      FPacGridPosition(4, 1)));
                        TestFalse(TEXT("normal step"),
                                  FPacGrid::IsWrapStep(FPacGridPosition(1, 1),
                                                       FPacGridPosition(2, 1)));
                    });

                 It("lets path finding use the tunnel as the shortest way",
                    [this]()
                    {
                        // The middle is a wall: the only route is through the
                        // tunnel (left from (1,1) and out of the right edge).
                        FPacGrid Grid = MakeGrid(TEXT("#######\n"
                                                      "--###--\n"
                                                      "#######"),
                                                 FIntPoint(7, 3));

                        EPacDirection Step = EPacDirection::None;

                        TestTrue(TEXT("route exists"),
                                 Grid.FindFirstStep(FPacGridPosition(1, 1),
                                                    FPacGridPosition(6, 1),
                                                    EPacDirection::None,
                                                    Step));
                        TestTrue(TEXT("first step is Left"),
                                 Step == EPacDirection::Left);

                        TestTrue(TEXT("and back again"),
                                 Grid.FindFirstStep(FPacGridPosition(5, 1),
                                                    FPacGridPosition(0, 1),
                                                    EPacDirection::None,
                                                    Step));
                        TestTrue(TEXT("first step is Right"),
                                 Step == EPacDirection::Right);
                    });
             });

    // ------------------------------------------------------------------
    Describe("GhostHouse",
             [this]()
             {
                 // Row y=1 is the street, (3,2) is the door tile, y=3 is the room.
                 constexpr TCHAR const* HouseLayout = TEXT(
                     "#######\n"
                     "#-----#\n"
                     "###H###\n"
                     "##HHH##\n"
                     "#######");
                 FIntPoint const HouseSize(7, 5);

                 It("parses H as a ghost house tile",
                    [this, HouseLayout, HouseSize]()
                    {
                        FPacGrid Grid = MakeGrid(HouseLayout, HouseSize);

                        TestTrue(TEXT("door tile is house"),
                                 Grid.GetTile(FPacGridPosition(3, 2)) ==
                                     EPacTileType::GhostHouse);
                        TestTrue(TEXT("IsGhostHouse"),
                                 Grid.IsGhostHouse(FPacGridPosition(2, 3)));
                        TestFalse(TEXT("street is not house"),
                                  Grid.IsGhostHouse(FPacGridPosition(3, 1)));
                        TestEqual(TEXT("house tile count"),
                                  Grid.CountTiles(EPacTileType::GhostHouse),
                                  4);
                    });

                 It("is not walkable by default, only when the house is "
                    "allowed",
                    [this, HouseLayout, HouseSize]()
                    {
                        FPacGrid Grid = MakeGrid(HouseLayout, HouseSize);

                        FPacGridPosition const Room(3, 3);

                        TestFalse(TEXT("default: closed"), Grid.IsWalkable(Room));
                        TestTrue(TEXT("allowed: open"),
                                 Grid.IsWalkable(Room, true));
                        TestTrue(TEXT("street is always walkable"),
                                 Grid.IsWalkable(FPacGridPosition(3, 1)));
                        TestFalse(TEXT("walls stay closed even when allowed"),
                                  Grid.IsWalkable(FPacGridPosition(0, 0), true));
                    });

                 It("keeps path finding out of the house unless allowed",
                    [this, HouseLayout, HouseSize]()
                    {
                        FPacGrid Grid = MakeGrid(HouseLayout, HouseSize);

                        EPacDirection Step = EPacDirection::None;

                        TestFalse(TEXT("street to room: no route by default"),
                                  Grid.FindFirstStep(FPacGridPosition(3, 1),
                                                     FPacGridPosition(3, 3),
                                                     EPacDirection::None,
                                                     Step));

                        TestTrue(TEXT("street to room: route when allowed"),
                                 Grid.FindFirstStep(FPacGridPosition(3, 1),
                                                    FPacGridPosition(3, 3),
                                                    EPacDirection::None,
                                                    Step,
                                                    true));
                        TestTrue(TEXT("goes through the door (Y + 1 is Up)"),
                                 Step == EPacDirection::Up);

                        TestFalse(TEXT("room to street: no route by default"),
                                  Grid.FindFirstStep(FPacGridPosition(3, 3),
                                                     FPacGridPosition(3, 1),
                                                     EPacDirection::None,
                                                     Step));

                        TestTrue(TEXT("room to street: route when allowed"),
                                 Grid.FindFirstStep(FPacGridPosition(3, 3),
                                                    FPacGridPosition(3, 1),
                                                    EPacDirection::None,
                                                    Step,
                                                    true));
                        TestTrue(TEXT("out through the door"),
                                 Step == EPacDirection::Down);
                    });

                 It("resolves a target inside the house only when allowed",
                    [this, HouseLayout, HouseSize]()
                    {
                        FPacGrid Grid = MakeGrid(HouseLayout, HouseSize);

                        FPacGridPosition Resolved;

                        TestTrue(TEXT("found by default"),
                                 Grid.FindNearestWalkable(FPacGridPosition(3, 3),
                                                          Resolved));
                        TestTrue(TEXT("default: pushed out to the street"),
                                 Resolved == FPacGridPosition(3, 1));

                        TestTrue(TEXT("found when allowed"),
                                 Grid.FindNearestWalkable(FPacGridPosition(3, 3),
                                                          Resolved,
                                                          true));
                        TestTrue(TEXT("allowed: the tile itself"),
                                 Resolved == FPacGridPosition(3, 3));
                    });
             });
}

#endif  // WITH_DEV_AUTOMATION_TESTS
