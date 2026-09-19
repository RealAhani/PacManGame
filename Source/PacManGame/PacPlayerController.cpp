// Fill out your copyright notice in the Description page of Project Settings.


#include "PacPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"

void APacPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    ULocalPlayer* LocalPlayer = GetLocalPlayer();

    if (LocalPlayer == nullptr)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
        LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

    if (InputSubsystem == nullptr)
    {
        return;
    }

    if (PlayerMappingContext == nullptr)
    {
        return;
    }

    InputSubsystem->AddMappingContext(PlayerMappingContext, 0);
}

void APacPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInputComponent =
        Cast<UEnhancedInputComponent>(InputComponent);

    if (EnhancedInputComponent == nullptr)
    {
        return;
    }

    if (MoveAction == nullptr)
    {
        return;
    }

    EnhancedInputComponent->BindAction(
        MoveAction,
        ETriggerEvent::Triggered,
        this,
        &APacPlayerController::HandleMove
    );
}

void APacPlayerController::HandleMove(const FInputActionValue& Value)
{
    const FVector2D MovementInput = Value.Get<FVector2D>();

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Move Input: X=%f Y=%f"),
        MovementInput.X,
        MovementInput.Y
    );
    MoveInput = MovementInput;
}
