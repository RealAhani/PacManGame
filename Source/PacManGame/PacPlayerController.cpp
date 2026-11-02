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
