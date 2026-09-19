#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PacPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS()
class PACMANGAME_API APacPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    FVector2D GetMoveInput() const { return MoveInput; }

protected:
    virtual void SetupInputComponent() override;

    void HandleMove(const struct FInputActionValue& Value);

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> PlayerMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

private:
    FVector2D MoveInput = FVector2D::ZeroVector;
};
