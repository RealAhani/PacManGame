#include "AndroidRefreshRateSubsystem.h"
#include "Engine/Engine.h"
#include "Misc/CoreDelegates.h"

#if PLATFORM_ANDROID
#include "Android/AndroidPlatformMisc.h"
// #include "Logging/PacManLog.h"
#endif

// this class is a subsystem so nothing need to do the engine do all the lifting
void UAndroidRefreshRateSubsystem::Initialize(FSubsystemCollectionBase & Collection)
{
    Super::Initialize(Collection);

#if PLATFORM_ANDROID
    ApplyRefreshRate();

    // The player may change "Motion smoothness" while the game is in the
    // background, so check again when the app comes back to the foreground
    ReactivatedHandle = FCoreDelegates::ApplicationHasReactivatedDelegate
                            .AddUObject(this, &UAndroidRefreshRateSubsystem::ApplyRefreshRate);
#endif
}

void UAndroidRefreshRateSubsystem::Deinitialize()
{
#if PLATFORM_ANDROID
    FCoreDelegates::ApplicationHasReactivatedDelegate.Remove(ReactivatedHandle);
#endif

    Super::Deinitialize();
}

void UAndroidRefreshRateSubsystem::ApplyRefreshRate()
{
#if PLATFORM_ANDROID
    int32 const DisplayHz = FAndroidMisc::GetNativeDisplayRefreshRate();

    // UE_LOG(LogPacMan, Log, TEXT("Pac-System : Display refresh rate: %d Hz"), DisplayHz);

    // Only act on displays faster than 60Hz; keep the engine default otherwise
    if (DisplayHz > 60 && GEngine)
    {
        // this fucking son of the bitch take 3h to found out , also make sure
        // do a correct cvars in DefaultDeviceProfiles.ini for android framepacing
        int32 const TargetHz = FMath::Min(DisplayHz, 120);
        GEngine->Exec(nullptr,
                      *FString::Printf(TEXT("r.setframepace %d"), TargetHz));

        // UE_LOG(LogPacMan, Log, TEXT("Pac-System : Frame pace set to %d"), TargetHz);
    }
#endif
}
