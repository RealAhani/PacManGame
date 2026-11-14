// Unit tests for FPacFruitFlashSettings (when the fruit flashes and how fast).
//
// Run in the editor: Tools > Session Frontend > Automation > "PacManGame.FruitFlash"

// clang-format off
#include "Misc/AutomationTest.h"
#include "../PacFruitFlash.h"
// clang-format on

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FPacFruitFlashSpec,
                  "PacManGame.FruitFlash",
                  EAutomationTestFlags::EditorContext |
                      EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FPacFruitFlashSpec)

void FPacFruitFlashSpec::Define()
{
    Describe("Stage",
             [this]()
             {
                 It("does not flash before the last 20%",
                    [this]()
                    {
                        FPacFruitFlashSettings const Flash;

                        TestEqual(TEXT("full lifetime"),
                                  Flash.GetStage(1.0f),
                                  static_cast<int32>(INDEX_NONE));
                        TestEqual(TEXT("just above 20%"),
                                  Flash.GetStage(0.21f),
                                  static_cast<int32>(INDEX_NONE));
                    });

                 It("starts at stage 0 when 20% is left",
                    [this]()
                    {
                        FPacFruitFlashSettings const Flash;

                        TestEqual(TEXT("exactly 20%"), Flash.GetStage(0.20f), 0);
                        TestEqual(TEXT("19%"), Flash.GetStage(0.19f), 0);
                    });

                 It("goes one stage up every 4%",
                    [this]()
                    {
                        FPacFruitFlashSettings const Flash;

                        TestEqual(TEXT("17%: 3% passed"), Flash.GetStage(0.17f), 0);
                        TestEqual(TEXT("15%: 5% passed"), Flash.GetStage(0.15f), 1);
                        TestEqual(TEXT("11%: 9% passed"), Flash.GetStage(0.11f), 2);
                        TestEqual(TEXT("7%: 13% passed"), Flash.GetStage(0.07f), 3);
                        TestEqual(TEXT("3%: 17% passed"), Flash.GetStage(0.03f), 4);
                        TestEqual(TEXT("0%: all 20% passed"),
                                  Flash.GetStage(0.0f),
                                  5);
                    });

                 It("treats a negative remaining time like zero",
                    [this]()
                    {
                        FPacFruitFlashSettings const Flash;

                        TestEqual(TEXT("past the end"),
                                  Flash.GetStage(-0.5f),
                                  Flash.GetStage(0.0f));
                    });

                 It("follows the settings",
                    [this]()
                    {
                        FPacFruitFlashSettings Flash;
                        Flash.StartRemainingFraction = 0.5f;
                        Flash.SpeedUpStepFraction    = 0.1f;

                        TestEqual(TEXT("before"),
                                  Flash.GetStage(0.6f),
                                  static_cast<int32>(INDEX_NONE));
                        TestEqual(TEXT("start"), Flash.GetStage(0.5f), 0);
                        TestEqual(TEXT("one step in"), Flash.GetStage(0.35f), 1);
                    });
             });

    Describe("Frequency",
             [this]()
             {
                 It("starts at the base frequency and gets faster each stage",
                    [this]()
                    {
                        FPacFruitFlashSettings Flash;
                        Flash.BaseFrequency     = 2.0f;
                        Flash.SpeedUpMultiplier = 1.5f;

                        TestTrue(TEXT("stage 0"),
                                 FMath::Abs(Flash.GetFrequency(0) - 2.0f) < 0.001f);
                        TestTrue(TEXT("stage 1"),
                                 FMath::Abs(Flash.GetFrequency(1) - 3.0f) < 0.001f);
                        TestTrue(TEXT("stage 2"),
                                 FMath::Abs(Flash.GetFrequency(2) - 4.5f) < 0.001f);
                    });
             });

    Describe("Advance",
             [this]()
             {
                 It("stays normal and resets while it is not flashing",
                    [this]()
                    {
                        FPacFruitFlashSettings const Flash;
                        float                        Phase = 0.7f;

                        TestTrue(TEXT("amount 0"),
                                 Flash.Advance(0.9f, 0.1f, Phase) == 0.0f);
                        TestTrue(TEXT("phase reset"), Phase == 0.0f);
                    });

                 It("goes normal -> white -> normal over one flash",
                    [this]()
                    {
                        // 1 flash per second, never faster.
                        FPacFruitFlashSettings Flash;
                        Flash.BaseFrequency     = 1.0f;
                        Flash.SpeedUpMultiplier = 1.0f;

                        float Phase = 0.0f;

                        float const Quarter = Flash.Advance(0.1f, 0.25f, Phase);
                        float const Half    = Flash.Advance(0.1f, 0.25f, Phase);
                        float const ThreeQ  = Flash.Advance(0.1f, 0.25f, Phase);
                        float const Full    = Flash.Advance(0.1f, 0.25f, Phase);

                        TestTrue(TEXT("rising at 1/4"),
                                 Quarter > 0.4f && Quarter < 0.6f);
                        TestTrue(TEXT("white at 1/2"), Half > 0.99f);
                        TestTrue(TEXT("falling at 3/4"),
                                 ThreeQ > 0.4f && ThreeQ < 0.6f);
                        TestTrue(TEXT("normal again after a full flash"),
                                 Full < 0.01f);
                    });

                 It("stays between 0 and 1",
                    [this]()
                    {
                        FPacFruitFlashSettings const Flash;
                        float                        Phase    = 0.0f;
                        bool                         bInRange = true;

                        for (int32 Frame = 0; Frame < 600; ++Frame)
                        {
                            float const Remaining = 0.2f -
                                                    0.2f * static_cast<float>(Frame) /
                                                        600.0f;
                            float const Amount = Flash.Advance(Remaining, 0.016f, Phase);

                            bInRange = bInRange && Amount >= 0.0f &&
                                       Amount <= 1.0f && Phase >= 0.0f &&
                                       Phase < 1.0f;
                        }

                        TestTrue(TEXT("always 0..1"), bInRange);
                    });

                 It("flashes more often in later stages",
                    [this]()
                    {
                        FPacFruitFlashSettings const Flash;

                        // Count how many flashes (phase wrap-arounds) fit into
                        // one second in an early and in a late stage.
                        auto const CountFlashes = [&Flash](float const Remaining)
                        {
                            float Phase   = 0.0f;
                            float Last    = 0.0f;
                            int32 Flashes = 0;

                            for (int32 Frame = 0; Frame < 1000; ++Frame)
                            {
                                Flash.Advance(Remaining, 0.001f, Phase);

                                if (Phase < Last)
                                {
                                    ++Flashes;
                                }
                                Last = Phase;
                            }

                            return Flashes;
                        };

                        int32 const Early = CountFlashes(0.19f);  // stage 0: 2.0 / s
                        int32 const Late = CountFlashes(0.03f);  // stage 4: ~7.7 / s

                        TestEqual(TEXT("stage 0 flashes about twice a second"),
                                  Early,
                                  1);
                        TestTrue(TEXT("stage 4 flashes much more often"), Late >= 7);
                    });
             });
}

#endif  // WITH_DEV_AUTOMATION_TESTS
