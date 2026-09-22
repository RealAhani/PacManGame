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
class PACMANGAME_API APacPlayerController : public APlayerController {
  GENERATED_BODY()

public:
  virtual void BeginPlay() override;

protected:
  virtual void SetupInputComponent() override;

private:
  // -------------------- functions --------------------
  void SubmitDirection(EPacDirection Direction);
  void HandleMoveUp(const FInputActionValue &Value);
  void HandleMoveDown(const FInputActionValue &Value);
  void HandleMoveLeft(const FInputActionValue &Value);
  void HandleMoveRight(const FInputActionValue &Value);
  void HandlePelletConsumed(int32 ScoreValue);

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
};
