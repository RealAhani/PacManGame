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

UCLASS()
class PACMANGAME_API APacPlayerController : public APlayerController
{
    GENERATED_BODY()

public:

    virtual void BeginPlay() override;

    void OnTouchPressed(ETouchIndex::Type Finger, FVector Location);
    void OnTouchMoved(ETouchIndex::Type Finger, FVector Location);
    void OnTouchReleased(ETouchIndex::Type Finger, FVector Location);


protected:

    virtual void SetupInputComponent() override;

private:

    // -------------------- functions --------------------
    void SubmitDirection(EPacDirection Direction);
    void HandleMoveUp(FInputActionValue const & Value);
    void HandleMoveDown(FInputActionValue const & Value);
    void HandleMoveLeft(FInputActionValue const & Value);
    void HandleMoveRight(FInputActionValue const & Value);

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

    FVector2D         SwipeStart     = FVector2D::ZeroVector;
    ETouchIndex::Type ActiveFinger   = ETouchIndex::Touch1;
    bool              bSwipeTracking = false;
};
