#include "engine/AudioEngine.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <memory>
#include <vector>

namespace snotes::engine
{
namespace
{

constexpr double kRate = 48000.0;
constexpr int kBlock = 64;

/// The engine with all its parts and a sound card whose output 0 runs through a cable of `delay` samples
/// into input 0 (the loopback test of Q-05); the delay is at least one block, as on a real device.
class LoopbackRig
{
public:
    explicit LoopbackRig(int delay) : cable_(static_cast<std::size_t>(delay), 0.0F) { engine.prepare(kRate); }

    void publish(std::vector<VoiceSnapshot> voices, double clickBpm = 0.0)
    {
        auto snapshot = std::make_unique<ProjectSnapshot>();
        snapshot->sampleRate = kRate;
        snapshot->clickBpm = clickBpm;
        snapshot->voices = std::move(voices);
        snapshots.publish(std::move(snapshot));
    }

    /// Runs blocks and returns what left output 0.
    std::vector<float> run(int numBlocks)
    {
        std::vector<float> played;
        std::array<float, kBlock> input{};
        std::array<float, kBlock> output{};
        for (int block = 0; block < numBlocks; ++block)
        {
            for (auto& sample : input)
            {
                sample = cable_.front();
                cable_.pop_front();
            }
            output.fill(1.0F); // stale data the engine must overwrite
            const std::array<const float*, 1> inputs{input.data()};
            std::array<float*, 1> outputs{output.data()};
            engine.process(inputs.data(), 1, outputs.data(), 1, kBlock);
            cable_.insert(cable_.end(), output.begin(), output.end());
            played.insert(played.end(), output.begin(), output.end());
        }
        return played;
    }

    SnapshotExchange<ProjectSnapshot> snapshots;
    Transport transport;
    InputMeter meter;
    Recorder recorder;
    TestToneSource tone{440.0, 0.5F};
    LatencyProbe probe;
    AudioEngine engine{{.snapshots = snapshots,
                        .transport = transport,
                        .meter = meter,
                        .recorder = recorder,
                        .tone = tone,
                        .probe = probe}};

private:
    std::deque<float> cable_;
};

std::shared_ptr<const TakeAudio> impulseAt(std::int64_t index, std::int64_t length)
{
    std::vector<float> samples(static_cast<std::size_t>(length), 0.0F);
    samples[static_cast<std::size_t>(index)] = 1.0F;
    return std::make_shared<const TakeAudio>(std::move(samples));
}

VoiceSnapshot playing(std::shared_ptr<const TakeAudio> audio, std::int64_t start, std::int64_t latency)
{
    VoiceSnapshot voice;
    voice.hasTake = true;
    voice.audio = std::move(audio);
    voice.takeStartSample = start;
    voice.takeLatencyOffsetSamples = latency;
    return voice;
}

std::vector<float> drain(AudioRingBuffer& ring)
{
    std::vector<float> samples(ring.readable());
    samples.resize(ring.read(samples.data(), samples.size()));
    return samples;
}

std::int64_t loudest(const std::vector<float>& samples)
{
    const auto peak =
        std::max_element(samples.begin(),
                         samples.end(),
                         [](float left, float right) { return std::abs(left) < std::abs(right); });
    return peak - samples.begin();
}

TEST(AudioEngineTest, FWI04_OverdubLinesUpWithThePlaybackAfterLatencyCompensation)
{
    constexpr int kLatency = 200;
    LoopbackRig rig{kLatency};
    rig.publish({playing(impulseAt(1000, 4000), 0, 0)});
    RecordingSession session{{0}};
    rig.recorder.start(session);
    rig.transport.play();

    rig.run(40);

    const auto recorded = drain(session.ring(0));
    ASSERT_EQ(session.startSample(), 0);
    EXPECT_EQ(loudest(recorded), 1000 + kLatency);

    // The overdub as a take: start and latency put its impulse on the reference's (Q-05).
    rig.transport.stop();
    rig.run(1);
    rig.transport.locate(990);
    rig.transport.play();
    rig.publish({playing(std::make_shared<const TakeAudio>(recorded), session.startSample(), kLatency)});
    const auto played = rig.run(1);
    EXPECT_EQ(loudest(played), 10);
    EXPECT_FLOAT_EQ(played[10], 1.0F);
}

TEST(AudioEngineTest, FWI04_RecordingStartsAtTheSongPosition)
{
    LoopbackRig rig{kBlock};
    rig.transport.locate(4800);
    rig.transport.play();
    rig.run(1);
    RecordingSession session{{0}};
    rig.recorder.start(session);

    rig.run(2);

    EXPECT_EQ(session.startSample(), 4800 + kBlock);
    EXPECT_EQ(session.recordedSamples(), 2 * kBlock);
}

TEST(AudioEngineTest, FWI01_NothingIsRecordedOrPlayedWhileStopped)
{
    LoopbackRig rig{kBlock};
    rig.publish({playing(impulseAt(0, 10), 0, 0)});
    RecordingSession session{{0}};
    rig.recorder.start(session);

    const auto played = rig.run(2);

    EXPECT_EQ(session.recordedSamples(), 0);
    EXPECT_EQ(session.startSample(), RecordingSession::kNotStarted);
    EXPECT_TRUE(std::all_of(played.begin(), played.end(), [](float sample) { return sample == 0.0F; }));
    EXPECT_EQ(rig.transport.position(), 0);
}

TEST(AudioEngineTest, FWI07_ClickPlaysWithTheTransport)
{
    LoopbackRig rig{kBlock};
    rig.publish({}, 120.0);
    rig.transport.play();

    const auto played = rig.run(1);

    EXPECT_NE(played[1], 0.0F);
}

TEST(AudioEngineTest, FWI06_MonitoringWorksWhileStopped)
{
    LoopbackRig rig{kBlock};
    VoiceSnapshot monitored;
    monitored.inputDi = 0;
    monitored.monitor = MonitorSource::di;
    rig.publish({monitored});
    rig.tone.setEnabled(true);
    const auto first = rig.run(1);
    rig.tone.setEnabled(false);

    const auto second = rig.run(1);

    // The second block hears the first (the tone) through the cable.
    EXPECT_GT(std::abs(first[5]), 0.0F);
    EXPECT_EQ(second, first);
}

TEST(AudioEngineTest, Q01_ProbeMeasuresTheLoopback)
{
    LoopbackRig rig{321};
    rig.probe.start(0);

    rig.run(2000);

    EXPECT_EQ(rig.probe.numFinished(), LatencyProbe::kPings);
    EXPECT_EQ(rig.probe.result(LatencyProbe::kPings - 1), 321);
}

TEST(AudioEngineTest, FAU03_MetersSeeTheInput)
{
    LoopbackRig rig{kBlock};
    rig.tone.setEnabled(true);

    rig.run(2);

    EXPECT_GT(rig.meter.takePeak(0), 0.0F);
}

} // namespace
} // namespace snotes::engine
