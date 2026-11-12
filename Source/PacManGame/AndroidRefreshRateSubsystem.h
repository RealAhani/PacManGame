#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AndroidRefreshRateSubsystem.generated.h"

// this class is a subsystem so nothing need to do the engine do all the lifting
// because the subsystem starts by itself. (Subsystems are created for the game
// instance in packaged builds, so it will also run on the phone.)
UCLASS()
class UAndroidRefreshRateSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    virtual void Initialize(FSubsystemCollectionBase & Collection) override;
    virtual void Deinitialize() override;

    // Reads the display refresh rate and applies the frame pace if it is above 60
    void ApplyRefreshRate();

private:

    FDelegateHandle ReactivatedHandle;
};
