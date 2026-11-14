#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "PacFruitFlash.generated.h"
// clang-format on

// The "about to disappear" warning of the bonus fruit: it flashes white and
// back to normal, and flashes faster and faster until it is eaten or gone.
//
// Plain maths, no actors: the manager asks it how white the fruit is right now.
//
// A STAGE is a step of the countdown. The last StartRemainingFraction of the
// lifetime is split into steps of SpeedUpStepFraction. Stage 0 is the first
// step, and every further stage flashes SpeedUpMultiplier times faster.
// Defaults for a 10 second fruit: flashing starts with 2 seconds left (20%),
// and every 0.4 seconds (4%) the flashing gets 1.4 times faster.
USTRUCT(BlueprintType)
struct PACMANGAME_API FPacFruitFlashSettings
{
    GENERATED_BODY()

    // Flashing starts when this fraction of the lifetime is left (0.2 = the
    // last 20%).
    UPROPERTY(EditAnywhere,
              BlueprintReadWrite,
              Category = "Flash",
              meta     = (ClampMin = "0.0", ClampMax = "1.0"))
    float StartRemainingFraction = 0.20f;

    // Every time this much more of the lifetime has passed, flashing gets
    // faster (0.04 = every 4%).
    UPROPERTY(EditAnywhere,
              BlueprintReadWrite,
              Category = "Flash",
              meta     = (ClampMin = "0.001", ClampMax = "1.0"))
    float SpeedUpStepFraction = 0.04f;

    // Flashes per second in the first stage. One flash = normal -> white ->
    // normal.
    UPROPERTY(EditAnywhere,
              BlueprintReadWrite,
              Category = "Flash",
              meta     = (ClampMin = "0.1"))
    float BaseFrequency = 2.0f;

    // Each stage multiplies the frequency by this (1 = never faster).
    UPROPERTY(EditAnywhere,
              BlueprintReadWrite,
              Category = "Flash",
              meta     = (ClampMin = "1.0"))
    float SpeedUpMultiplier = 1.4f;

    // INDEX_NONE (-1) while it is not flashing yet, else 0, 1, 2...
    int32 GetStage(float const RemainingFraction) const
    {
        if (RemainingFraction > StartRemainingFraction)
        {
            return INDEX_NONE;
        }

        float const Passed = StartRemainingFraction -
                             FMath::Max(RemainingFraction, 0.0f);

        // The tiny extra keeps a step boundary from rounding down.
        return FMath::FloorToInt(
            Passed / FMath::Max(SpeedUpStepFraction, 0.001f) + 0.0001f);
    }

    float GetFrequency(int32 const Stage) const
    {
        return BaseFrequency *
               FMath::Pow(SpeedUpMultiplier, static_cast<float>(Stage));
    }

    // Moves the flash on by DeltaSeconds and returns how white the fruit is
    // (0 = normal, 1 = fully white). InOutPhase is the position in the current
    // flash (0..1); keep it between calls. It is advanced, not calculated from
    // the time, so the flash does not jump when the frequency goes up.
    float Advance(float const RemainingFraction,
                  float const DeltaSeconds,
                  float &     InOutPhase) const
    {
        int32 const Stage = GetStage(RemainingFraction);

        if (Stage == INDEX_NONE)
        {
            InOutPhase = 0.0f;
            return 0.0f;
        }

        InOutPhase = FMath::Fmod(InOutPhase + GetFrequency(Stage) * DeltaSeconds,
                                 1.0f);

        // Smooth 0 -> 1 -> 0: starts and ends at normal.
        return 0.5f - 0.5f * FMath::Cos(2.0f * PI * InOutPhase);
    }
};
