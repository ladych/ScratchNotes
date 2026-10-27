#include "engine/PlaybackMixer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <vector>

namespace snotes::engine
{
namespace
{

using ::testing::Each;
using ::testing::ElementsAre;

std::shared_ptr<const TakeAudio> take(std::vector<float> samples)
{
    return std::make_shared<const TakeAudio>(std::move(samples));
}

VoiceSnapshot voiceWith(std::shared_ptr<const TakeAudio> audio)
{
    VoiceSnapshot voice;
    voice.hasTake = true;
    voice.audio = std::move(audio);
    return voice;
}

/// Two outputs of four samples plus an inactive third one.
struct Outputs
{
    std::array<float, 4> left{};
    std::array<float, 4> right{};
    std::array<float*, 3> channels{left.data(), right.data(), nullptr};
};

TEST(PlaybackMixerTest, FWI02_PlaysTheTakeOnAllOutputs)
{
    ProjectSnapshot snapshot;
    snapshot.voices.push_back(voiceWith(take({0.1F, 0.2F, 0.3F, 0.4F})));
    PlaybackMixer mixer;
    Outputs out;

    mixer.renderTakes(snapshot, 0, out.channels.data(), 3, 4);

    EXPECT_THAT(out.left, ElementsAre(0.1F, 0.2F, 0.3F, 0.4F));
    EXPECT_EQ(out.left, out.right);
}

TEST(PlaybackMixerTest, FWI05_TakePlaysFromItsStartMinusTheLatency)
{
    ProjectSnapshot snapshot;
    auto voice = voiceWith(take({1.0F, 2.0F, 3.0F}));
    voice.takeStartSample = 10;
    voice.takeLatencyOffsetSamples = 3;
    snapshot.voices.push_back(voice);
    PlaybackMixer mixer;
    Outputs out;

    mixer.renderTakes(snapshot, 5, out.channels.data(), 1, 4); // song samples 5..8, take starts at 7

    EXPECT_THAT(out.left, ElementsAre(0.0F, 0.0F, 1.0F, 2.0F));
}

TEST(PlaybackMixerTest, FWI03_GainAndMasterGainScaleTheTake)
{
    ProjectSnapshot snapshot;
    snapshot.masterGain = 0.5F;
    auto voice = voiceWith(take({1.0F, 1.0F, 1.0F, 1.0F}));
    voice.gain = 0.5F;
    snapshot.voices.push_back(voice);
    PlaybackMixer mixer;
    Outputs out;

    mixer.renderTakes(snapshot, 0, out.channels.data(), 1, 4);

    EXPECT_THAT(out.left, Each(0.25F));
}

TEST(PlaybackMixerTest, FWI03_InaudibleVoicesAndVoicesWithoutAudioAreSilent)
{
    ProjectSnapshot snapshot;
    auto muted = voiceWith(take({1.0F, 1.0F, 1.0F, 1.0F}));
    muted.audible = false;
    snapshot.voices.push_back(muted);
    snapshot.voices.push_back(voiceWith(nullptr));
    PlaybackMixer mixer;
    Outputs out;

    mixer.renderTakes(snapshot, 0, out.channels.data(), 1, 4);

    EXPECT_THAT(out.left, Each(0.0F));
}

TEST(PlaybackMixerTest, FWI02_VoicesAreMixedAndLongBlocksPlayInChunks)
{
    constexpr int kLength = PlaybackMixer::kChunk + 3;
    ProjectSnapshot snapshot;
    snapshot.voices.push_back(voiceWith(take(std::vector<float>(kLength, 0.25F))));
    snapshot.voices.push_back(voiceWith(take(std::vector<float>(kLength, 0.5F))));
    PlaybackMixer mixer;
    std::vector<float> samples(kLength, 0.0F);
    std::array<float*, 1> outputs{samples.data()};

    mixer.renderTakes(snapshot, 0, outputs.data(), 1, kLength);

    EXPECT_THAT(samples, Each(0.75F));
}

TEST(PlaybackMixerTest, FWI06_MonitorsTheChosenInput)
{
    std::array<float, 4> di{0.1F, 0.1F, 0.1F, 0.1F};
    std::array<float, 4> wet{0.2F, 0.2F, 0.2F, 0.2F};
    const std::array<const float*, 3> inputs{di.data(), wet.data(), nullptr};
    ProjectSnapshot snapshot;
    VoiceSnapshot processed;
    processed.inputDi = 0;
    processed.inputWet = 1;
    processed.monitor = MonitorSource::processed;
    VoiceSnapshot dry;
    dry.inputDi = 0;
    dry.inputWet = 1;
    dry.monitor = MonitorSource::di;
    dry.gain = 2.0F;
    snapshot.voices = {processed, dry};
    Outputs out;

    PlaybackMixer::renderMonitoring(snapshot, inputs.data(), 3, out.channels.data(), 3, 4);

    EXPECT_THAT(out.left, Each(::testing::FloatEq(0.4F)));
}

TEST(PlaybackMixerTest, FWI06_UnmonitoredOrUnroutedInputsAreNotHeard)
{
    std::array<float, 4> di{0.1F, 0.1F, 0.1F, 0.1F};
    const std::array<const float*, 2> inputs{di.data(), nullptr};
    ProjectSnapshot snapshot;
    VoiceSnapshot notMonitored;
    notMonitored.inputDi = 0;
    VoiceSnapshot noInput;
    noInput.inputDi = 0;
    noInput.monitor = MonitorSource::processed;
    VoiceSnapshot missingInput;
    missingInput.inputDi = 7;
    missingInput.monitor = MonitorSource::di;
    VoiceSnapshot inactiveInput;
    inactiveInput.inputDi = 1;
    inactiveInput.monitor = MonitorSource::di;
    snapshot.voices = {notMonitored, noInput, missingInput, inactiveInput};
    Outputs out;

    PlaybackMixer::renderMonitoring(snapshot, inputs.data(), 2, out.channels.data(), 1, 4);

    EXPECT_THAT(out.left, Each(0.0F));
}

TEST(PlaybackMixerTest, FNA05_TheTranscriptionPlaysInsteadOfTheTakes)
{
    ProjectSnapshot snapshot;
    snapshot.sampleRate = 1000.0;
    snapshot.masterGain = 0.5F;
    snapshot.playsTranscription = true;
    auto voice = voiceWith(take({0.1F, 0.2F, 0.3F, 0.4F}));
    voice.gain = 0.5F;
    const auto notes = std::make_shared<const SynthNotes>(
        SynthNotes{.notes = {{.start = 0, .end = 100, .frequency = 110.0}}, .longest = 100});
    voice.notes = notes;
    snapshot.voices.push_back(voice);
    // A voice without notes is silent, as is a muted one.
    snapshot.voices.push_back(voiceWith(take({1.0F, 1.0F, 1.0F, 1.0F})));
    auto muted = voice;
    muted.audible = false;
    snapshot.voices.push_back(muted);
    PlaybackMixer mixer;
    Outputs out;

    mixer.renderTakes(snapshot, 1, out.channels.data(), 3, 4);

    for (std::size_t index = 0; index < out.left.size(); ++index)
        EXPECT_FLOAT_EQ(
            out.left[index],
            0.25F * NoteSynth::sampleAt(notes->notes.front(), 1 + static_cast<std::int64_t>(index), 1000.0));
    EXPECT_EQ(out.left, out.right);
}

} // namespace
} // namespace snotes::engine
