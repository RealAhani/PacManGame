// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacGhostManager.h"
#include "Logging/PacManLog.h"
#include "PacGhostDecisionComponent.h"
#include "PacGhostMovementComponent.h"
#include "PacGhostAIController.h"
#include "PacGhostPawn.h"
#include "PacMaze.h"
#include "PacGridMovementComponent.h"
#include "PacMazeDataAsset.h"

#include "DrawDebugHelpers.h"

// clang-format on

APacGhostManager::APacGhostManager()
{
    PrimaryActorTick.bCanEverTick = true;

    // Classic timing: scatter 7, chase 20, scatter 7, chase 20, scatter 5,
    // chase 20, scatter 5, then chase forever.
    auto const AddPhase =
        [this](EPacGhostBehaviorMode const Mode, float const Seconds)
    {
        FPacModePhase Phase;
        Phase.Mode    = Mode;
        Phase.Seconds = Seconds;
        ModeSchedule.Add(Phase);
    };

    AddPhase(EPacGhostBehaviorMode::Scatter, 7.0f);
    AddPhase(EPacGhostBehaviorMode::Chase, 20.0f);
    AddPhase(EPacGhostBehaviorMode::Scatter, 7.0f);
    AddPhase(EPacGhostBehaviorMode::Chase, 20.0f);
    AddPhase(EPacGhostBehaviorMode::Scatter, 5.0f);
    AddPhase(EPacGhostBehaviorMode::Chase, 20.0f);
    AddPhase(EPacGhostBehaviorMode::Scatter, 5.0f);
    AddPhase(EPacGhostBehaviorMode::Chase, 0.0f);
}

void APacGhostManager::Tick(float const DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

#if ENABLE_DRAW_DEBUG
    if (bShowDebugModes)
    {
        DrawDebugModes();
    }
#endif


    if (!bRunning)
    {
        return;
    }

    ReviveArrivedGhosts();

    if (FrightenedTimeLeft > 0.0f)
    {
        // The Scatter/Chase timer is paused while frightened.
        FrightenedTimeLeft -= DeltaSeconds;

        if (FrightenedTimeLeft <= 0.0f)
        {
            EndFrightened();
        }
        return;
    }

    if (!bUseStateTree)
    {
        AdvanceSchedule(DeltaSeconds);
    }
}


void APacGhostManager::Initialize(APacMaze*                  InMaze,
                                  UPacGridMovementComponent* InPlayerMovement)
{
    check(InMaze);
    check(InPlayerMovement);

    Maze           = InMaze;
    PlayerMovement = InPlayerMovement;

    SpawnGhosts();
}

void APacGhostManager::SpawnGhosts()
{
    check(Maze);

    if (GhostClass == nullptr)
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: GhostManager has no GhostClass assigned."));
        return;
    }

    UPacGridMovementComponent* PlayerMovementComponent = PlayerMovement;


    TArray<FPacGridPosition> const & GhostStarts = Maze->GetGhostStarts();
    GhostStartPositions                          = GhostStarts;


    UPacGhostMovementComponent* BlinkyMovementComponent = nullptr;

    FIntPoint const GridSize = Maze->GetGridSize();

    TArray<FPacGridPosition> const ScatterTargets =
        {FPacGridPosition(1, 1),
         FPacGridPosition(GridSize.X - 2, 1),
         FPacGridPosition(1, GridSize.Y - 2),
         FPacGridPosition(GridSize.X - 2, GridSize.Y - 2)};

    for (int32 Index = 0; Index < GhostStarts.Num(); ++Index)
    {
        FPacGridPosition const StartPosition = GhostStarts[Index];

        FVector const WorldPosition = Maze->GridToWorld(StartPosition);

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.Owner = this;

        APacGhostPawn* Ghost = GetWorld()->SpawnActor<APacGhostPawn>(GhostClass,
                                                                     WorldPosition,
                                                                     FRotator::ZeroRotator,
                                                                     SpawnParameters);

        if (Ghost == nullptr)
        {
            UE_LOG(LogPacMan,
                   Error,
                   TEXT("PAC: Failed to spawn ghost %d at (%d,%d)."),
                   Index,
                   StartPosition.X,
                   StartPosition.Y);

            continue;
        }

        UPacGhostMovementComponent* MovementComponent = Ghost->GetGhostMovementComponent();

        if (MovementComponent == nullptr)
        {
            UE_LOG(LogPacMan,
                   Error,
                   TEXT("PAC: Ghost %d has no movement component."),
                   Index);

            Ghost->Destroy();
            continue;
        }

        MovementComponent->InitializeMaze(Maze, StartPosition);

        UPacGhostDecisionComponent* DecisionComponent = Ghost->GetGhostDecisionComponent();

        if (DecisionComponent == nullptr)
        {
            UE_LOG(LogPacMan,
                   Error,
                   TEXT("PAC: Ghost %d has no decision component."),
                   Index);

            Ghost->Destroy();
            continue;
        }

        DecisionComponent->Initialize(Maze, MovementComponent);

        if (Index == 0)
        {
            BlinkyMovementComponent = MovementComponent;
        }
        DecisionComponent->SetBlinkyMovementComponent(BlinkyMovementComponent);

        EPacGhostPersonality const Personality = static_cast<EPacGhostPersonality>(
            Index % 4);

        DecisionComponent->SetPersonality(Personality);

        DecisionComponent->SetPlayerMovementComponent(PlayerMovementComponent);

        FPacGridPosition ScatterTarget = ScatterTargets[Index % ScatterTargets.Num()];

        if (!Maze->IsWalkable(ScatterTarget))
        {
            UE_LOG(LogPacMan,
                   Warning,
                   TEXT("PAC: Scatter target (%d,%d) is blocked; "
                        "using ghost start position instead."),
                   ScatterTarget.X,
                   ScatterTarget.Y);

            ScatterTarget = StartPosition;
        }

        DecisionComponent->SetScatterTarget(ScatterTarget);
        DecisionComponent->SetHomePosition(StartPosition);

        if (bUseStateTree)
        {
            // The StateTree is started when play begins (SetMovementEnabled),
            // so its round timers do not run during the Ready pause. Until
            // then the ghost just waits in the first round's mode.
            if (Cast<APacGhostAIController>(Ghost->GetController()) == nullptr)
            {
                UE_LOG(LogPacMan,
                       Error,
                       TEXT("PAC: Ghost %d has no PacGhostAIController. "
                            "StateTree cannot started."),
                       Index);
            }
        }

        DecisionComponent->SetBehaviorMode(GetBaseMode());

        Ghosts.Add(Ghost);

        UE_LOG(LogPacMan,
               Log,
               TEXT("PAC: Ghost %d initialized at (%d,%d)."),
               Index,
               StartPosition.X,
               StartPosition.Y);
    }

    RestartSchedule();
    bStateTreesNeedStart = true;

    UE_LOG(LogPacMan, Log, TEXT("PAC: Spawned %d ghosts."), Ghosts.Num());
}


// ---------------------------------------------------------------------------
// Mode schedule
// ---------------------------------------------------------------------------

EPacGhostBehaviorMode APacGhostManager::GetBaseMode() const
{
    return ModeSchedule.IsValidIndex(PhaseIndex) ? ModeSchedule[PhaseIndex].Mode
                                                 : EPacGhostBehaviorMode::Chase;
}

// Tells every ghost the current Scatter / Chase mode. Frightened or Eaten
// ghosts only remember it; they use it once released.
void APacGhostManager::ApplyBaseMode()
{
    for (APacGhostPawn* Ghost : Ghosts)
    {
        if (IsValid(Ghost) && Ghost->GetGhostDecisionComponent() != nullptr)
        {
            Ghost->GetGhostDecisionComponent()->SetBehaviorMode(GetBaseMode());
        }
    }
}

void APacGhostManager::RestartSchedule()
{
    PhaseIndex = 0;
    PhaseTimeLeft = ModeSchedule.IsValidIndex(0) ? ModeSchedule[0].Seconds : 0.0f;
    FrightenedTimeLeft = 0.0f;
    GhostsEatenInChain = 0;
}

void APacGhostManager::AdvanceSchedule(float const DeltaSeconds)
{
    if (!ModeSchedule.IsValidIndex(PhaseIndex) ||
        ModeSchedule[PhaseIndex].Seconds <= 0.0f ||  // "forever" phase
        !ModeSchedule.IsValidIndex(PhaseIndex + 1))
    {
        return;
    }

    PhaseTimeLeft -= DeltaSeconds;

    if (PhaseTimeLeft > 0.0f)
    {
        return;
    }

    ++PhaseIndex;
    PhaseTimeLeft = ModeSchedule[PhaseIndex].Seconds;

    UE_LOG(LogPacMan,
           Log,
           TEXT("PAC: Ghost phase %d -> %s"),
           PhaseIndex,
           *UEnum::GetValueAsString(GetBaseMode()));

    ApplyBaseMode();
}


void APacGhostManager::StartFrightened()
{
    FrightenedTimeLeft = FrightenedSeconds;
    GhostsEatenInChain = 0;

    for (APacGhostPawn* Ghost : Ghosts)
    {
        UPacGhostDecisionComponent* Decision = IsValid(Ghost)
                                                   ? Ghost->GetGhostDecisionComponent()
                                                   : nullptr;

        // An eaten ghost keeps running home.
        if (Decision != nullptr &&
            Decision->GetBehaviorMode() != EPacGhostBehaviorMode::Eaten)
        {
            Decision->SetBehaviorMode(EPacGhostBehaviorMode::Frightened);
        }
    }

    // Classic: the round timers stand still while ghosts are frightened.
    SetStateTreesPaused(true);
    UE_LOG(LogPacMan, Log, TEXT("PAC: Ghosts frightened for %.1fs."), FrightenedSeconds);
}

void APacGhostManager::EndFrightened()
{
    FrightenedTimeLeft = 0.0f;
    GhostsEatenInChain = 0;

    for (APacGhostPawn* Ghost : Ghosts)
    {
        UPacGhostDecisionComponent* Decision = IsValid(Ghost)
                                                   ? Ghost->GetGhostDecisionComponent()
                                                   : nullptr;

        if (Decision != nullptr &&
            Decision->GetBehaviorMode() == EPacGhostBehaviorMode::Frightened)
        {
            // Back to whatever round the schedule / StateTree is in now.
            Decision->ReleaseOverride();
        }
    }
    SetStateTreesPaused(false);
}

// An eaten ghost that got back to its start tile comes back to life.
void APacGhostManager::ReviveArrivedGhosts()
{
    for (int32 Index = 0; Index < Ghosts.Num(); ++Index)
    {
        APacGhostPawn* Ghost = Ghosts[Index];

        if (!IsValid(Ghost) || Ghost->GetGhostMovementComponent() == nullptr ||
            Ghost->GetGhostDecisionComponent() == nullptr)

        {
            continue;
        }

        UPacGhostDecisionComponent* Decision = Ghost->GetGhostDecisionComponent();

        if (Decision->GetBehaviorMode() == EPacGhostBehaviorMode::Eaten &&
            Ghost->GetGhostMovementComponent()->GetCurrentGridPosition() ==
                GhostStartPositions[Index])
        {
            // Still frightened globally? Come back frightened, else normal.
            if (FrightenedTimeLeft > 0.0f)
            {
                Decision->SetBehaviorMode(EPacGhostBehaviorMode::Frightened);
            }
            else
            {
                Decision->ReleaseOverride();
            }
        }
    }
}
// ---------------------------------------------------------------------------
// Contact, enable, reset
// ---------------------------------------------------------------------------

bool APacGhostManager::UpdateContacts(FVector const & PlayerLocation,
                                      float const     HitDistance)
{
    float const HitDistanceSquared = HitDistance * HitDistance;
    bool        bPlayerCaught      = false;

    for (APacGhostPawn* Ghost : Ghosts)
    {
        UPacGhostDecisionComponent* Decision = IsValid(Ghost)
                                                   ? Ghost->GetGhostDecisionComponent()
                                                   : nullptr;

        if (Decision == nullptr ||
            FVector::DistSquared2D(Ghost->GetActorLocation(), PlayerLocation) >
                HitDistanceSquared)
        {
            continue;
        }

        if (Decision->GetBehaviorMode() == EPacGhostBehaviorMode::Frightened)
        {
            Decision->SetBehaviorMode(EPacGhostBehaviorMode::Eaten);

            // 200, 400, 800, 1600 for ghosts eaten with one power pellet.
            int32 const Score = GhostScoreBase << FMath::Min(GhostsEatenInChain, 3);
            ++GhostsEatenInChain;

            UE_LOG(LogPacMan, Log, TEXT("PAC: Ghost eaten. Score=%d"), Score);
            OnGhostEaten.Broadcast(Score);
        }
        else if (Decision->IsDangerous())
        {
            bPlayerCaught = true;
        }
    }

    return bPlayerCaught;
}

void APacGhostManager::SetMovementEnabled(bool const bEnabled)
{
    bRunning = bEnabled;

    // Play begins: (re)start the StateTrees now, at their Root (first round).
    if (bEnabled && bStateTreesNeedStart)
    {
        bStateTreesNeedStart = false;
        RestartStateTrees();
    }

    for (APacGhostPawn* Ghost : Ghosts)
    {
        if (IsValid(Ghost) && Ghost->GetGhostMovementComponent() != nullptr)
        {
            Ghost->GetGhostMovementComponent()->SetMovementEnabled(bEnabled);
        }
    }
}

void APacGhostManager::ResetGhosts()
{
    check(Maze);
    check(Ghosts.Num() == GhostStartPositions.Num());

    RestartSchedule();

    for (int32 Index = 0; Index < Ghosts.Num(); ++Index)
    {
        APacGhostPawn* Ghost = Ghosts[Index];

        if (!IsValid(Ghost))
        {
            continue;
        }

        UPacGhostMovementComponent* Movement = Ghost->GetGhostMovementComponent();
        UPacGhostDecisionComponent* Decision = Ghost->GetGhostDecisionComponent();

        if (Movement == nullptr || Decision == nullptr)
        {
            continue;
        }

        Movement->InitializeMaze(Maze, GhostStartPositions[Index]);

        // A new life starts at the first round (Scatter). Remember it as the
        // base mode, then leave Frightened / Eaten if the ghost was in them.
        Decision->SetBehaviorMode(
            bUseStateTree ? EPacGhostBehaviorMode::Scatter : GetBaseMode());
        Decision->ReleaseOverride();
        Decision->Respawn();
    }

    // Stop the trees now; they start again at their Root (Scatter1) when the
    // Ready pause ends and play begins.
    StopStateTrees();
    bStateTreesNeedStart = true;
}

void APacGhostManager::SetStateTreesPaused(bool const bPaused)
{
    if (!bUseStateTree)
    {
        return;
    }

    for (APacGhostPawn* Ghost : Ghosts)
    {
        if (!IsValid(Ghost))
        {
            continue;
        }

        if (APacGhostAIController* AIController = Cast<APacGhostAIController>(
                Ghost->GetController()))
        {
            AIController->SetStateTreePaused(bPaused);
        }
    }
}

void APacGhostManager::StopStateTrees()
{
    if (!bUseStateTree)
    {
        return;
    }

    for (APacGhostPawn* Ghost : Ghosts)
    {
        if (!IsValid(Ghost))
        {
            continue;
        }

        if (APacGhostAIController* AIController = Cast<APacGhostAIController>(
                Ghost->GetController()))
        {
            AIController->StopStateTree();
        }
    }
}


void APacGhostManager::RestartStateTrees()
{
    if (!bUseStateTree)
    {
        return;
    }

    for (APacGhostPawn* Ghost : Ghosts)
    {
        if (!IsValid(Ghost))
        {
            continue;
        }

        if (APacGhostAIController* AIController = Cast<APacGhostAIController>(
                Ghost->GetController()))
        {
            AIController->RestartStateTree();
        }
    }
}

void APacGhostManager::DrawDebugModes() const
{
#if ENABLE_DRAW_DEBUG
    UEnum const* const ModeEnum = StaticEnum<EPacGhostBehaviorMode>();

    for (APacGhostPawn const* Ghost : Ghosts)
    {
        UPacGhostDecisionComponent const*
            Decision = IsValid(Ghost) ? Ghost->GetGhostDecisionComponent() : nullptr;

        if (Decision == nullptr)
        {
            continue;
        }

        EPacGhostBehaviorMode const Mode = Decision->GetBehaviorMode();

        FString Text = ModeEnum->GetNameStringByValue(static_cast<int64>(Mode));

        FColor Color = FColor::White;
        switch (Mode)
        {
            case EPacGhostBehaviorMode::Scatter:
                Color = FColor::Cyan;
                break;
            case EPacGhostBehaviorMode::Chase:
                Color = FColor::Red;
                break;
            case EPacGhostBehaviorMode::Frightened:
                Color = FColor::Blue;
                Text += FString::Printf(TEXT(" %.1f"), FrightenedTimeLeft);
                break;
            case EPacGhostBehaviorMode::Eaten:
                Color = FColor::Silver;
                break;
            default:
                break;
        }

        DrawDebugString(GetWorld(),
                        Ghost->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f),
                        Text,
                        nullptr,
                        Color,
                        0.0f,
                        true,
                        1.2f);
    }
#endif
}
