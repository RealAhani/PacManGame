#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PacGridTypes.h"
#include "PacPlayerController.generated.h"
// clang-format on

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

// A short touch that did not move: where on the screen (pixels) and where on
// the maze plane (world units).
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPacTap,
                                     FVector2D /*ScreenPosition*/,
                                     FVector /*WorldPosition*/);

// Two fingers moved. Scale > 1: fingers moved apart (zoom in).
// Scale < 1: fingers moved together (zoom out). Centre is the screen position
// between the fingers.
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPacPinch,
                                     float /*Scale*/,
                                     FVector2D /*ScreenCentre*/);

// How the player steers on a touch screen. Tap and pinch work in both.
UENUM(BlueprintType)
enum class EPacTouchScheme : uint8
{
    Swipe,     // drag a finger anywhere
    Joystick,  // one virtual stick on one side of the screen
};

// Which half of the screen the virtual stick lives in.
UENUM(BlueprintType)
enum class EPacJoystickSide : uint8
{
    Left,
    Right,
};


UCLASS()
class PACMANGAME_API APacPlayerController : public APlayerController
{
    GENERATED_BODY()

public:

    virtual void BeginPlay() override;

    void OnTouchPressed(ETouchIndex::Type Finger, FVector Location);
    void OnTouchMoved(ETouchIndex::Type Finger, FVector Location);
    void OnTouchReleased(ETouchIndex::Type Finger, FVector Location);

    // The plane taps are projected onto: a point on it and its normal. Defaults
    // fit an unrotated maze at the world origin. For other mazes call
    // SetTapPlane(Maze->GetActorLocation(), Maze->GetActorUpVector()).
    void SetTapPlane(FVector const & PlanePoint, FVector const & PlaneNormal);

    // Screen pixels -> point on the tap plane. False if the touch ray misses it.
    bool ScreenToWorldOnPlane(FVector2D const & ScreenPosition,
                              FVector &         OutWorld) const;

    FOnPacTap   OnTap;
    FOnPacPinch OnPinch;

    // -------------------- touch settings (call these from the menu) --------------------
    // Both are saved to GameUserSettings.ini and loaded again at start-up.
    UFUNCTION(BlueprintCallable, Category = "Input|Touch")
    void SetTouchScheme(EPacTouchScheme NewScheme);

    UFUNCTION(BlueprintPure, Category = "Input|Touch")
    EPacTouchScheme GetTouchScheme() const
    {
        return TouchScheme;
    }

    UFUNCTION(BlueprintCallable, Category = "Input|Touch")
    void SetJoystickSide(EPacJoystickSide NewSide);

    UFUNCTION(BlueprintPure, Category = "Input|Touch")
    EPacJoystickSide GetJoystickSide() const
    {
        return JoystickSide;
    }

    // For the HUD: where to draw the stick (screen pixels). Returns false when
    // the joystick scheme is off. While no finger is down, the base is drawn at
    // its resting place as a hint of where to touch.
    // The HUD tells the controller the size it draws in, every frame, so the
    // stick is placed with the same numbers it is drawn with.
    void SetJoystickViewSize(FVector2D const & ViewSize)
    {
        HudViewSize = ViewSize;
    }

    // For the HUD: where to draw the stick (screen pixels). Returns false when
    // the joystick scheme is off. While no finger is down, the base is drawn at
    // its resting place as a hint of where to touch.
    bool GetJoystickVisual(FVector2D & OutBase,
                           FVector2D & OutThumb,
                           float &     OutRadius,
                           bool &      bOutActive) const;


protected:

    virtual void SetupInputComponent() override;

private:

    // -------------------- functions --------------------
    void SubmitDirection(EPacDirection Direction);
    void HandleMoveUp(FInputActionValue const & Value);
    void HandleMoveDown(FInputActionValue const & Value);
    void HandleMoveLeft(FInputActionValue const & Value);
    void HandleMoveRight(FInputActionValue const & Value);

    // joystick
    void  StartJoystick(ETouchIndex::Type Finger, FVector2D const & Position);
    void  UpdateJoystick(FVector2D const & Position);
    void  EndJoystick();
    bool  IsInJoystickZone(FVector2D const & Position) const;
    float GetJoystickRadius() const;
    FVector2D GetViewSize() const;
    FVector2D ClampJoystickCenter(FVector2D const & Position) const;
    FVector2D GetJoystickRestPosition() const;
    void      LoadTouchSettings();
    void      SaveTouchSettings() const;

    void  BeginPinch();
    void  UpdatePinch();
    float GetFingerDistance() const;
    float GetMinViewportSide() const;

    // -------------------- properties --------------------
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> PlayerMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveUpAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveDownAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveLeftAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveRightAction;

    EPacDirection LastSubmittedDirection = EPacDirection::None;

    UPROPERTY(EditDefaultsOnly,
              Category = "Input|Swipe",
              meta     = (ClampMin = "0.01", ClampMax = "0.5"))
    float MinSwipeFraction = 0.06f;

    // A touch is a tap if the finger moved less than this (fraction of the
    // shorter screen side, so it is the same on every phone density)...
    UPROPERTY(EditDefaultsOnly,
              Category = "Input|Tap",
              meta     = (ClampMin = "0.001", ClampMax = "0.1"))
    float MaxTapMoveFraction = 0.02f;

    // ...and was released within this many seconds.
    UPROPERTY(EditDefaultsOnly,
              Category = "Input|Tap",
              meta     = (ClampMin = "0.05", ClampMax = "1.0"))
    float MaxTapSeconds = 0.25f;

    // The finger distance must change by at least this fraction before a pinch
    // event is sent (filters finger jitter).
    UPROPERTY(EditDefaultsOnly,
              Category = "Input|Pinch",
              meta     = (ClampMin = "0.0", ClampMax = "0.2"))
    float PinchDeadZone = 0.01f;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Touch")
    EPacTouchScheme TouchScheme = EPacTouchScheme::Joystick;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Joystick")
    EPacJoystickSide JoystickSide = EPacJoystickSide::Right;

    // Stick radius as a fraction of the shorter screen side.
    UPROPERTY(EditDefaultsOnly,
              Category = "Input|Joystick",
              meta     = (ClampMin = "0.05", ClampMax = "0.3"))
    float JoystickRadiusFraction = 0.12f;

    // Fraction of the radius the thumb must leave the centre before it steers.
    UPROPERTY(EditDefaultsOnly,
              Category = "Input|Joystick",
              meta     = (ClampMin = "0.0", ClampMax = "0.9"))
    float JoystickDeadZone = 0.3f;

    // Where the stick rests before a finger touches it: this far from the
    // screen edge on its own side (fraction of the screen width), and this far
    // down from the top (fraction of the screen height).
    UPROPERTY(EditDefaultsOnly,
              Category = "Input|Joystick",
              meta     = (ClampMin = "0.0", ClampMax = "0.5"))
    float JoystickRestEdgeFraction = 0.25f;

    UPROPERTY(EditDefaultsOnly,
              Category = "Input|Joystick",
              meta     = (ClampMin = "0.0", ClampMax = "1.0"))
    float JoystickRestHeightFraction = 0.62f;

    FVector2D HudViewSize = FVector2D::ZeroVector;

    bool              bJoystickActive = false;
    ETouchIndex::Type JoystickFinger  = ETouchIndex::Touch1;
    FVector2D         JoystickCenter  = FVector2D::ZeroVector;
    FVector2D         JoystickThumb   = FVector2D::ZeroVector;

    FVector2D         SwipeStart     = FVector2D::ZeroVector;
    ETouchIndex::Type ActiveFinger   = ETouchIndex::Touch1;
    bool              bSwipeTracking = false;
    bool              bSwipeFired = false;  // a swipe already sent a direction
    double            TouchStartTime = 0.0;

    // Touch1 / Touch2 only (enough for a pinch).
    FVector2D FingerPosition[2] = {FVector2D::ZeroVector, FVector2D::ZeroVector};
    bool  bFingerDown[2]    = {false, false};
    bool  bPinching         = false;
    float LastPinchDistance = 0.0f;

    FVector TapPlanePoint  = FVector::ZeroVector;
    FVector TapPlaneNormal = FVector::UpVector;
};
