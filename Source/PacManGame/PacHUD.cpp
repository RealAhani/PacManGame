// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacHUD.h"
#include "Logging/PacManLog.h"
#include "PacHUDWidget.h"
#include "PacPlayerState.h"
#include "PacGameMode.h"
#include "PacPlayerController.h"

#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Canvas.h"
// clang-format on

void APacHUD::BeginPlay()
{
    Super::BeginPlay();

    if (HUDWidgetClass == nullptr)
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: HUDWidgetClass is not assigned."));
        return;
    }

    HUDWidget = CreateWidget<UPacHUDWidget>(GetWorld(), HUDWidgetClass);

    if (HUDWidget == nullptr)
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: Failed to create HUD widget."));
        return;
    }

    HUDWidget->AddToViewport();

    APlayerController* PlayerController = GetOwningPlayerController();

    if (PlayerController == nullptr)
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: HUD has no owning PlayerController."));
        return;
    }

    APacPlayerState* PacPlayerState = Cast<APacPlayerState>(
        PlayerController->PlayerState);

    if (PacPlayerState == nullptr)
    {
        UE_LOG(LogPacMan, Error, TEXT("PAC: HUD could not find APacPlayerState."));
        return;
    }

    // Set the initial value.
    HUDWidget->SetScore(PacPlayerState->GetCurrentScore());
    HUDWidget->SetLives(PacPlayerState->GetLives());


    // Listen for future changes.
    PacPlayerState->OnScoreChanged.AddUObject(this, &APacHUD::HandleScoreChanged);
    PacPlayerState->OnLivesChanged.AddUObject(this, &APacHUD::HandleLivesChanged);

    if (APacGameMode* GameMode = GetWorld()->GetAuthGameMode<APacGameMode>())
    {
        GameMode->OnGameOver.AddUObject(this, &APacHUD::HandleGameOver);
        GameMode->OnLevelCompelet.AddUObject(this, &APacHUD::HandleLevelCompelet);
    }
}

void APacHUD::DrawHUD()
{
    Super::DrawHUD();

    APacPlayerController* PC = Cast<APacPlayerController>(
        GetOwningPlayerController());
    if (PC == nullptr || Canvas == nullptr)
    {
        return;
    }
    PC->SetJoystickViewSize(FVector2D(Canvas->SizeX, Canvas->SizeY));

    FVector2D Base, Thumb;
    float     Radius  = 0.0f;
    bool      bActive = false;

    if (!PC->GetJoystickVisual(Base, Thumb, Radius, bActive))
    {
        return;  // swipe scheme: nothing to draw
    }

    // UE_LOG(LogPacMan,
    //        Error,
    //        TEXT("PAC: Joystick Base=(%.0f,%.0f) Thumb=(%.0f,%.0f) R=%.0f "
    //             "Active=%d Canvas=(%d,%d)"),
    //        Base.X,
    //        Base.Y,
    //        Thumb.X,
    //        Thumb.Y,
    //        Radius,
    //        bActive ? 1 : 0,
    //        Canvas->SizeX,
    //        Canvas->SizeY);

    float const Alpha = bActive ? 1.0f : 0.5f;

    // Draw Joystick
    // its draw the texture for joystick but if its null it draws polygon instead
    // Option 1 :Draw polygon for joystick
    // Option 2 : Draw Imgage for joystick
    auto const DrawCircleImage =
        [this](UTexture2D* Texture, FVector2D Centre, float Diameter, float Alpha)
    {
        FVector2D const Size(Diameter, Diameter);
        Canvas->K2_DrawTexture(Texture,
                               Centre - Size * 0.5,
                               Size,
                               FVector2D(0, 0),
                               FVector2D(1, 1),
                               FLinearColor(1, 1, 1, Alpha),
                               BLEND_Translucent);
    };

    if (JoystickBaseTexture)
    {
        DrawCircleImage(JoystickBaseTexture, Base, Radius * 2.0f, Alpha);
    }
    else
    {
        Canvas->K2_DrawPolygon(nullptr,
                               Base,
                               FVector2D(Radius, Radius),
                               48,
                               FLinearColor(1.f, 1.f, 1.f, 0.15f * Alpha));
    }

    if (JoystickThumbTexture)
    {
        DrawCircleImage(JoystickThumbTexture, Thumb, Radius * 0.8f, Alpha);
    }
    else
    {
        Canvas->K2_DrawPolygon(nullptr,
                               Thumb,
                               FVector2D(Radius * 0.4f, Radius * 0.4f),
                               32,
                               FLinearColor(1.f, 1.f, 1.f, 0.45f * Alpha));
    }
}

void APacHUD::HandleLivesChanged(int32 const Lives)
{
    if (HUDWidget != nullptr)
    {
        HUDWidget->SetLives(Lives);
    }
}

void APacHUD::HandleGameOver()
{
    if (HUDWidget != nullptr)
    {
        HUDWidget->ShowGameOver();
    }
}

void APacHUD::HandleLevelCompelet()
{
    if (HUDWidget != nullptr)
    {
        HUDWidget->ShowWinning();
    }
}

void APacHUD::HandleScoreChanged(int32 const Score)
{
    if (HUDWidget == nullptr)
    {
        return;
    }

    HUDWidget->SetScore(Score);
}
