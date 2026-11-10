// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacPlayerController.h"
#include "Logging/PacManLog.h"
#include "PacPlayerPawn.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
// clang-format on

void APacPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Input only. Camera, pawn setup, pellets and score are wired by
    // APacGameMode::InitializeLevel() in one fixed order, so this controller
    // does not depend on when the GameMode's BeginPlay runs.
    if (!IsLocalController())
    {
        return;
    }

    if (PlayerMappingContext == nullptr)
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: PlayerMappingContext is not assigned on '%s'. "
                    "Set it in the controller Blueprint."),
               *GetName());
        return;
    }
    ULocalPlayer* LocalPlayer = GetLocalPlayer();

    UEnhancedInputLocalPlayerSubsystem*
        InputSubsystem = LocalPlayer != nullptr
                             ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()
                             : nullptr;

    if (InputSubsystem == nullptr)
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: No Enhanced Input subsystem on '%s'."),
               *GetName());
        return;
    }

    InputSubsystem->AddMappingContext(PlayerMappingContext, 0);
}

void APacPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(
        InputComponent);

    if (EnhancedInputComponent == nullptr)
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: Input component is not UEnhancedInputComponent. Set "
                    "Default Input Component Class in Project Settings."));
        return;
    }

    if (MoveUpAction == nullptr || MoveDownAction == nullptr ||
        MoveLeftAction == nullptr || MoveRightAction == nullptr)
    {
        UE_LOG(LogPacMan,
               Error,
               TEXT("PAC: A Move input action is not assigned on '%s'."),
               *GetName());
        return;
    }

    EnhancedInputComponent->BindAction(MoveUpAction,
                                       ETriggerEvent::Triggered,
                                       this,
                                       &APacPlayerController::HandleMoveUp);

    EnhancedInputComponent->BindAction(MoveDownAction,
                                       ETriggerEvent::Triggered,
                                       this,
                                       &APacPlayerController::HandleMoveDown);

    EnhancedInputComponent->BindAction(MoveLeftAction,
                                       ETriggerEvent::Triggered,
                                       this,
                                       &APacPlayerController::HandleMoveLeft);

    EnhancedInputComponent->BindAction(MoveRightAction,
                                       ETriggerEvent::Triggered,
                                       this,
                                       &APacPlayerController::HandleMoveRight);

    InputComponent->BindTouch(IE_Pressed, this, &APacPlayerController::OnTouchPressed);
    InputComponent->BindTouch(IE_Repeat, this, &APacPlayerController::OnTouchMoved);
    InputComponent->BindTouch(IE_Released, this, &APacPlayerController::OnTouchReleased);
}

void APacPlayerController::SubmitDirection(EPacDirection const Direction)
{

    APacPlayerPawn* PlayerPawn = Cast<APacPlayerPawn>(GetPawn());

    if (PlayerPawn == nullptr)
    {
        return;
    }

    if (PlayerPawn->IsMovementBlocked())
    {
        LastSubmittedDirection = EPacDirection::None;
    }

    if (Direction == LastSubmittedDirection)
    {
        return;
    }

    LastSubmittedDirection = Direction;

    PlayerPawn->SetDesiredDirection(Direction);

    UE_LOG(LogPacMan,
           Log,
           TEXT("PAC: Direction submitted: %d"),
           static_cast<uint8>(Direction));
}

void APacPlayerController::HandleMoveUp(FInputActionValue const & Value)
{
    SubmitDirection(EPacDirection::Up);
}

void APacPlayerController::HandleMoveDown(FInputActionValue const & Value)
{
    SubmitDirection(EPacDirection::Down);
}

void APacPlayerController::HandleMoveLeft(FInputActionValue const & Value)
{
    SubmitDirection(EPacDirection::Left);
}

void APacPlayerController::HandleMoveRight(FInputActionValue const & Value)
{
    SubmitDirection(EPacDirection::Right);
}

// Android-Input
//
void APacPlayerController::OnTouchPressed(ETouchIndex::Type Finger, FVector Location)
{
    if (bSwipeTracking)
        return;  // ignore extra fingers
    ActiveFinger   = Finger;
    SwipeStart     = FVector2D(Location.X, Location.Y);
    bSwipeTracking = true;
}

void APacPlayerController::OnTouchMoved(ETouchIndex::Type Finger, FVector Location)
{
    if (!bSwipeTracking || Finger != ActiveFinger)
        return;

    FVector2D const Current(Location.X, Location.Y);
    FVector2D const Delta = Current - SwipeStart;

    int32 SizeX = 0, SizeY = 0;
    GetViewportSize(SizeX, SizeY);
    float const MinDist = FMath::Min(SizeX, SizeY) * MinSwipeFraction;

    if (Delta.SizeSquared() < MinDist * MinDist)
        return;

    // Screen Y points down
    EPacDirection const
        Dir = FMath::Abs(Delta.X) > FMath::Abs(Delta.Y)
                  ? (Delta.X > 0.f ? EPacDirection::Left : EPacDirection::Right)
                  : (Delta.Y > 0.f ? EPacDirection::Down : EPacDirection::Up);

    SubmitDirection(Dir);
    SwipeStart = Current;  // lets the player chain swipes without lifting the finger
}

void APacPlayerController::OnTouchReleased(ETouchIndex::Type Finger, FVector Location)
{
    if (Finger == ActiveFinger)
        bSwipeTracking = false;
}
