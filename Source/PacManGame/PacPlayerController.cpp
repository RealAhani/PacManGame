// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacPlayerController.h"
#include "Logging/PacManLog.h"
#include "PacPlayerPawn.h"
#include "PacTouchMenuWidget.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/KismetSystemLibrary.h"
// clang-format on

namespace
{
// Screen Y points down.
EPacDirection DirectionFromScreenDelta(FVector2D const & Delta)
{
    return FMath::Abs(Delta.X) > FMath::Abs(Delta.Y)
               ? (Delta.X > 0.f ? EPacDirection::Left : EPacDirection::Right)
               : (Delta.Y > 0.f ? EPacDirection::Down : EPacDirection::Up);
}
}  // namespace

void APacPlayerController::BeginPlay()
{
    Super::BeginPlay();

    LoadTouchSettings();

    // Input only. Camera, pawn setup, pellets and score are wired by
    // APacGameMode::InitializeLevel() in one fixed order, so this controller
    // does not depend on when the GameMode's BeginPlay runs.
    if (!IsLocalController())
    {
        return;
    }

    if (TouchMenuWidgetClass != nullptr)
    {
        TouchMenuWidget = CreateWidget<UPacTouchMenuWidget>(this, TouchMenuWidgetClass);
        if (TouchMenuWidget != nullptr)
        {
            TouchMenuWidget->AddToViewport(10);  // above the HUD
        }
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

#if PLATFORM_ANDROID
    InputComponent->BindKey(EKeys::Android_Back,
                            IE_Pressed,
                            this,
                            &APacPlayerController::OnAndroidBackPressed);

#endif
}

void APacPlayerController::OnAndroidBackPressed()
{
    UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
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
// One finger: swipe (change direction) or tap.
// Two fingers: pinch. A second finger cancels the swipe / tap of the first.
//
// The size the HUD draws in (set every frame), or the viewport size before the
// HUD has drawn once.
FVector2D APacPlayerController::GetViewSize() const
{
    if (HudViewSize.X > 0.0 && HudViewSize.Y > 0.0)
    {
        return HudViewSize;
    }

    int32 SizeX = 0, SizeY = 0;
    GetViewportSize(SizeX, SizeY);
    return FVector2D(SizeX, SizeY);
}

float APacPlayerController::GetMinViewportSide() const
{
    FVector2D const ViewSize = GetViewSize();
    float const     SizeX    = static_cast<float>(ViewSize.X);
    float const     SizeY    = static_cast<float>(ViewSize.Y);
    return FMath::Min(SizeX, SizeY);
}

void APacPlayerController::OnTouchPressed(ETouchIndex::Type Finger, FVector Location)
{
    FVector2D const Position(Location.X, Location.Y);

    int32 const FingerIndex = static_cast<int32>(Finger);
    if (FingerIndex < 2)
    {
        FingerPosition[FingerIndex] = Position;
        bFingerDown[FingerIndex]    = true;
    }

    // Joystick scheme: a touch that starts on the stick's side is the stick.
    if (TouchScheme == EPacTouchScheme::Joystick && !bJoystickActive &&
        !bPinching && IsInJoystickZone(Position))
    {
        StartJoystick(Finger, Position);
        return;
    }

    // Second finger while the first is down: pinch, not swipe.
    if (!bJoystickActive && bSwipeTracking && Finger != ActiveFinger &&
        FingerIndex < 2 && bFingerDown[0] && bFingerDown[1])
    {
        BeginPinch();
        return;
    }

    if (bSwipeTracking || bPinching)
    {
        return;  // ignore extra fingers
    }

    ActiveFinger   = Finger;
    SwipeStart     = Position;
    bSwipeTracking = true;
    bSwipeFired    = false;
    TouchStartTime = GetWorld() != nullptr ? GetWorld()->GetRealTimeSeconds() : 0.0;
}

void APacPlayerController::OnTouchMoved(ETouchIndex::Type Finger, FVector Location)
{
    FVector2D const Current(Location.X, Location.Y);

    int32 const FingerIndex = static_cast<int32>(Finger);
    if (FingerIndex < 2)
    {
        FingerPosition[FingerIndex] = Current;
    }

    if (bJoystickActive && Finger == JoystickFinger)
    {
        UpdateJoystick(Current);
        return;
    }

    if (bPinching)
    {
        UpdatePinch();
        return;
    }

    if (!bSwipeTracking || Finger != ActiveFinger)
        return;

    // With the joystick scheme a moving finger outside the stick does nothing
    // (it can still be a tap).
    if (TouchScheme != EPacTouchScheme::Swipe)
        return;

    FVector2D const Delta = Current - SwipeStart;

    float const MinDist = GetMinViewportSide() * MinSwipeFraction;

    if (Delta.SizeSquared() < MinDist * MinDist)
        return;

    SubmitDirection(DirectionFromScreenDelta(Delta));
    SwipeStart = Current;  // lets the player chain swipes without lifting the finger
    bSwipeFired = true;  // this touch is a swipe, never a tap
}

void APacPlayerController::OnTouchReleased(ETouchIndex::Type Finger, FVector Location)
{
    FVector2D const Position(Location.X, Location.Y);

    int32 const FingerIndex = static_cast<int32>(Finger);
    if (FingerIndex < 2)
    {
        bFingerDown[FingerIndex] = false;
    }

    if (bJoystickActive && Finger == JoystickFinger)
    {
        EndJoystick();
        return;
    }

    // Lifting either finger ends the pinch. The finger that stays down does
    // not start a swipe: a new touch is needed.
    if (bPinching)
    {
        if (FingerIndex < 2)
        {
            bPinching      = false;
            bSwipeTracking = false;
        }
        return;
    }

    if (Finger != ActiveFinger || !bSwipeTracking)
    {
        return;
    }

    bSwipeTracking = false;

    // A short touch that never moved far and never swiped is a tap.
    double const Held = (GetWorld() != nullptr ? GetWorld()->GetRealTimeSeconds()
                                               : 0.0) -
                        TouchStartTime;
    float const MaxMove = GetMinViewportSide() * MaxTapMoveFraction;

    // SwipeStart is reset on every swipe, but bSwipeFired already excludes those.
    if (bSwipeFired || Held > MaxTapSeconds ||
        (Position - SwipeStart).Size() > MaxMove)
    {
        return;
    }

    FVector World;
    if (ScreenToWorldOnPlane(Position, World))
    {
        UE_LOG(LogPacMan,
               Verbose,
               TEXT("PAC: Tap screen (%.0f,%.0f) world (%.1f,%.1f,%.1f)"),
               Position.X,
               Position.Y,
               World.X,
               World.Y,
               World.Z);

        OnTap.Broadcast(Position, World);
    }
}

// -------------------- pinch --------------------

float APacPlayerController::GetFingerDistance() const
{
    return static_cast<float>((FingerPosition[0] - FingerPosition[1]).Size());
}

void APacPlayerController::BeginPinch()
{
    bPinching         = true;
    bSwipeTracking    = false;  // a pinch is never a swipe or a tap
    LastPinchDistance = GetFingerDistance();
}

void APacPlayerController::UpdatePinch()
{
    if (!bFingerDown[0] || !bFingerDown[1] || LastPinchDistance <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    float const Distance = GetFingerDistance();
    float const Scale    = Distance / LastPinchDistance;

    if (FMath::Abs(Scale - 1.0f) < PinchDeadZone)
    {
        return;
    }

    LastPinchDistance = Distance;

    FVector2D const Centre = (FingerPosition[0] + FingerPosition[1]) * 0.5;

    OnPinch.Broadcast(Scale, Centre);
}

// -------------------- screen -> world --------------------

void APacPlayerController::SetTapPlane(FVector const & PlanePoint,
                                       FVector const & PlaneNormal)
{
    TapPlanePoint  = PlanePoint;
    TapPlaneNormal = PlaneNormal.GetSafeNormal();
}

// Casts a ray from the camera through the touched pixel and intersects it with
// the maze plane. Works for perspective and orthographic cameras and needs no
// collision on the maze.
bool APacPlayerController::ScreenToWorldOnPlane(FVector2D const & ScreenPosition,
                                                FVector & OutWorld) const
{
    FVector Origin, Direction;

    if (!DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, Origin, Direction))
    {
        return false;
    }

    float const Denominator = FVector::DotProduct(Direction, TapPlaneNormal);

    if (FMath::IsNearlyZero(Denominator))
    {
        return false;  // ray runs parallel to the plane
    }

    float const T = FVector::DotProduct(TapPlanePoint - Origin, TapPlaneNormal) /
                    Denominator;

    if (T < 0.0f)
    {
        return false;  // the plane is behind the camera
    }

    OutWorld = Origin + Direction * T;
    return true;
}

// -------------------- joystick --------------------

float APacPlayerController::GetJoystickRadius() const
{
    return GetMinViewportSide() * JoystickRadiusFraction;
}

bool APacPlayerController::IsInJoystickZone(FVector2D const & Position) const
{
    FVector2D const ViewSize = GetViewSize();
    float const     SizeX    = static_cast<float>(ViewSize.X);
    float const     SizeY    = static_cast<float>(ViewSize.Y);

    float const Middle = static_cast<float>(SizeX) * 0.5f;

    return JoystickSide == EPacJoystickSide::Right
               ? Position.X >= Middle
               : Position.X < Middle;
}

// Keeps the whole stick on screen and inside its half.
FVector2D APacPlayerController::ClampJoystickCenter(FVector2D const & Position) const
{
    FVector2D const ViewSize = GetViewSize();
    float const     SizeX    = static_cast<float>(ViewSize.X);
    float const     SizeY    = static_cast<float>(ViewSize.Y);

    float const Radius = GetJoystickRadius();
    float const Middle = static_cast<float>(SizeX) * 0.5f;
    bool const  bRight = JoystickSide == EPacJoystickSide::Right;

    float const MinX = bRight ? Middle + Radius : Radius;
    float const MaxX = bRight ? static_cast<float>(SizeX) - Radius : Middle - Radius;
    float const MaxY = static_cast<float>(SizeY) - Radius;

    return FVector2D(FMath::Clamp<float>(static_cast<float>(Position.X),
                                         MinX,
                                         FMath::Max(MinX, MaxX)),
                     FMath::Clamp<float>(static_cast<float>(Position.Y),
                                         Radius,
                                         FMath::Max(Radius, MaxY)));
}

FVector2D APacPlayerController::GetJoystickRestPosition() const
{
    FVector2D const ViewSize = GetViewSize();
    float const     SizeX    = static_cast<float>(ViewSize.X);
    float const     SizeY    = static_cast<float>(ViewSize.Y);

    float const Edge    = JoystickRestEdgeFraction;
    float const CentreX = static_cast<float>(SizeX) *
                          (JoystickSide == EPacJoystickSide::Right ? 1.0f - Edge
                                                                   : Edge);

    return ClampJoystickCenter(
        FVector2D(CentreX, static_cast<float>(SizeY) * JoystickRestHeightFraction));
}

// A floating stick: its base appears where the finger lands.
void APacPlayerController::StartJoystick(ETouchIndex::Type const Finger,
                                         FVector2D const &       Position)
{
    bJoystickActive = true;
    JoystickFinger  = Finger;
    JoystickCenter  = ClampJoystickCenter(Position);
    JoystickThumb   = JoystickCenter;

    UpdateJoystick(Position);
}

void APacPlayerController::UpdateJoystick(FVector2D const & Position)
{
    double const Radius = GetJoystickRadius();

    FVector2D    Offset = Position - JoystickCenter;
    double const Length = Offset.Size();

    if (Length > Radius && Length > KINDA_SMALL_NUMBER)
    {
        Offset *= Radius / Length;
    }

    JoystickThumb = JoystickCenter + Offset;

    // Past the dead zone: steer. Back in the middle: keep going the same way.
    if (Length >= Radius * JoystickDeadZone)
    {
        SubmitDirection(DirectionFromScreenDelta(Offset));
    }
}

void APacPlayerController::EndJoystick()
{
    bJoystickActive = false;
}

bool APacPlayerController::GetJoystickVisual(FVector2D & OutBase,
                                             FVector2D & OutThumb,
                                             float &     OutRadius,
                                             bool &      bOutActive) const
{
    if (TouchScheme != EPacTouchScheme::Joystick)
    {
        return false;
    }

    OutRadius  = GetJoystickRadius();
    bOutActive = bJoystickActive;
    OutBase    = bJoystickActive ? JoystickCenter : GetJoystickRestPosition();
    OutThumb   = bJoystickActive ? JoystickThumb : OutBase;
    return true;
}

// -------------------- settings --------------------

namespace
{
TCHAR const* const TouchSection = TEXT("PacMan.Touch");
}

void APacPlayerController::SetTouchScheme(EPacTouchScheme const NewScheme)
{
    if (NewScheme == TouchScheme)
    {
        return;
    }

    TouchScheme = NewScheme;

    // Drop any touch that is in progress; the player lifts and touches again.
    bJoystickActive = false;
    bSwipeTracking  = false;
    bPinching       = false;

    SaveTouchSettings();
}

void APacPlayerController::SetJoystickSide(EPacJoystickSide const NewSide)
{
    if (NewSide == JoystickSide)
    {
        return;
    }

    JoystickSide    = NewSide;
    bJoystickActive = false;

    SaveTouchSettings();
}

void APacPlayerController::LoadTouchSettings()
{
    if (GConfig == nullptr)
    {
        return;
    }

    int32 Scheme = static_cast<int32>(TouchScheme);
    int32 Side   = static_cast<int32>(JoystickSide);

    GConfig->GetInt(TouchSection, TEXT("Scheme"), Scheme, GGameUserSettingsIni);
    GConfig->GetInt(TouchSection, TEXT("JoystickSide"), Side, GGameUserSettingsIni);

    if (Scheme >= 0 && Scheme <= static_cast<int32>(EPacTouchScheme::Joystick))
    {
        TouchScheme = static_cast<EPacTouchScheme>(Scheme);
    }

    if (Side >= 0 && Side <= static_cast<int32>(EPacJoystickSide::Right))
    {
        JoystickSide = static_cast<EPacJoystickSide>(Side);
    }
}

void APacPlayerController::SaveTouchSettings() const
{
    if (GConfig == nullptr)
    {
        return;
    }

    GConfig->SetInt(TouchSection,
                    TEXT("Scheme"),
                    static_cast<int32>(TouchScheme),
                    GGameUserSettingsIni);
    GConfig->SetInt(TouchSection,
                    TEXT("JoystickSide"),
                    static_cast<int32>(JoystickSide),
                    GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}
