#include "ui/AudioStatus.h"

#include "i18n/TestLanguage.h"

#include <gtest/gtest.h>

namespace snotes::ui
{
namespace
{

AudioDeviceInfo jack128()
{
    return {.open = true,
            .driver = "JACK",
            .device = "system",
            .sampleRate = 48000.0,
            .bufferSize = 128,
            .inputLatencySamples = 128,
            .outputLatencySamples = 256,
            .inputChannels = 4,
            .outputChannels = 2};
}

TEST(AudioStatusTest, Q02_LatenciesAreTheDriversValues)
{
    EXPECT_NEAR(inputLatencyMs(jack128()), 2.667, 1.0e-3);
    EXPECT_NEAR(outputLatencyMs(jack128()), 5.333, 1.0e-3);
}

TEST(AudioStatusTest, Q02_WithoutDriverValueEachLatencyIsOneBuffer)
{
    auto info = jack128();
    info.inputLatencySamples = 0;
    info.outputLatencySamples = 0;
    EXPECT_NEAR(inputLatencyMs(info), 2.667, 1.0e-3);
    EXPECT_NEAR(outputLatencyMs(info), 2.667, 1.0e-3);
}

TEST(AudioStatusTest, Q01_RoundTripIsInputPlusOutputLatency)
{
    EXPECT_NEAR(roundTripLatencyMs(jack128()), 8.0, 1.0e-9);
}

TEST(AudioStatusTest, Q02_StatusLineShowsDeviceBufferLatenciesAndChannels)
{
    EXPECT_EQ(audioStatusText(jack128()),
              "JACK | system | 48000 Hz | Buffer 128 (2,7 ms) | Roundtrip 8,0 ms (Eingang 2,7 ms, Ausgang "
              "5,3 ms) | "
              "4 Eingänge, 2 Ausgänge");
}

TEST(AudioStatusTest, FUI01_StatusLineInEnglishUsesDecimalPoints)
{
    const i18n::test::ScopedEnglish english;
    EXPECT_EQ(
        audioStatusText(jack128()),
        "JACK | system | 48000 Hz | Buffer 128 (2.7 ms) | Round trip 8.0 ms (input 2.7 ms, output 5.3 ms) | "
        "4 inputs, 2 outputs");
}

TEST(AudioStatusTest, Q01_MarksARoundTripAboveTenMilliseconds)
{
    auto info = jack128();
    info.outputLatencySamples = 384;
    EXPECT_NE(audioStatusText(info).find("Roundtrip 10,7 ms über 10 ms (Eingang"), std::string::npos);
    info.outputLatencySamples = 352; // exactly 10 ms
    EXPECT_EQ(audioStatusText(info).find("über 10 ms"), std::string::npos);
}

TEST(AudioStatusTest, FAU01_WarnsWhenTheDeviceHasNoInputs)
{
    auto info = jack128();
    info.inputChannels = 0;
    EXPECT_NE(audioStatusText(info).find("| Kein Eingang, Aufnahme nicht möglich, 2 Ausgänge"),
              std::string::npos);
}

TEST(AudioStatusTest, Q02_ClosedDevice)
{
    AudioDeviceInfo info;
    EXPECT_EQ(audioStatusText(info), "Kein Audio-Gerät geöffnet");
    info.open = true;
    EXPECT_EQ(audioStatusText(info), "Kein Audio-Gerät geöffnet");
}

TEST(AudioStatusTest, Q03_ShowsDropoutsWhenTheDriverCountsThem)
{
    auto info = jack128();
    info.xruns = 3;
    EXPECT_EQ(audioStatusText(info),
              "JACK | system | 48000 Hz | Buffer 128 (2,7 ms) | Roundtrip 8,0 ms (Eingang 2,7 ms, Ausgang "
              "5,3 ms) | "
              "4 Eingänge, 2 Ausgänge | XRuns: 3");
    info.xruns = 0;
    EXPECT_NE(audioStatusText(info).find("| XRuns: 0"), std::string::npos);
}

TEST(AudioStatusTest, Q09_WarnsWhileTheDeviceGivesNoAudio)
{
    auto info = jack128();
    info.stalled = true;
    EXPECT_EQ(audioStatusText(info).rfind("Audio-Gerät antwortet nicht, neuer Versuch läuft | JACK | ", 0),
              0U);
    AudioDeviceInfo closed;
    closed.stalled = true;
    EXPECT_EQ(audioStatusText(closed),
              "Audio-Gerät antwortet nicht, neuer Versuch läuft | Kein Audio-Gerät geöffnet");
}

TEST(AudioStatusTest, FAO03_NamesTheDeviceAFallbackReplaces)
{
    auto info = jack128();
    info.driver = "Windows Audio";
    info.device = "Lautsprecher";
    info.replacedDevice = "ASIO: SSL 12 ASIO Driver";
    EXPECT_EQ(audioStatusText(info).rfind(
                  "Ersatz für ASIO: SSL 12 ASIO Driver | Windows Audio | Lautsprecher | ", 0),
              0U);
}

} // namespace
} // namespace snotes::ui
