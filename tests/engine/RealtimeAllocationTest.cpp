#include "engine/AudioEngine.h"
#include "engine/InputMeter.h"
#include "engine/Recorder.h"
#include "engine/RecordingSession.h"
#include "engine/SpscQueue.h"
#include "engine/TestToneSource.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <memory>
#include <new>
#include <vector>

namespace
{

// Counts heap allocations of the current thread while counting is switched on.
thread_local bool countAllocations = false;
thread_local int allocationCount = 0;

} // namespace

void* operator new(std::size_t size)
{
    if (countAllocations)
        ++allocationCount;
    if (void* memory = std::malloc(size == 0 ? 1 : size))
        return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t /*size*/) noexcept
{
    std::free(memory);
}

namespace snotes::engine
{
namespace
{

TEST(RealtimeAllocationTest, Q04_TestToneDoesNotAllocate)
{
    TestToneSource tone{440.0, 0.2F};
    tone.prepare(48000.0);
    tone.setEnabled(true);
    std::array<float, 128> left{};
    std::array<float, 128> right{};
    const std::array<float*, 2> channels{left.data(), right.data()};

    countAllocations = true;
    for (int block = 0; block < 200; ++block)
        tone.render(channels.data(), 2, 128);
    countAllocations = false;

    EXPECT_EQ(allocationCount, 0);
}

TEST(RealtimeAllocationTest, Q04_QueueDoesNotAllocate)
{
    SpscQueue<int, 64> queue;
    int value = 0;

    countAllocations = true;
    for (int round = 0; round < 1000; ++round)
    {
        queue.push(round);
        queue.pop(value);
    }
    countAllocations = false;

    EXPECT_EQ(allocationCount, 0);
    EXPECT_EQ(value, 999);
}

TEST(RealtimeAllocationTest, Q04_RecordingDoesNotAllocate)
{
    Recorder recorder;
    InputMeter meter;
    RecordingSession session{{0, 1}};
    std::array<float, 128> left{};
    std::array<float, 128> right{};
    const std::array<const float*, 2> channels{left.data(), right.data()};
    std::array<float, 128> drained{};
    recorder.start(session);

    countAllocations = true;
    for (int block = 0; block < 200; ++block)
    {
        meter.process(channels.data(), 2, 128);
        recorder.process(channels.data(), 2, 128, 0);
        static_cast<void>(session.ring(0).read(drained.data(), drained.size()));
        static_cast<void>(session.ring(1).read(drained.data(), drained.size()));
    }
    countAllocations = false;

    EXPECT_EQ(allocationCount, 0);
    EXPECT_EQ(session.recordedSamples(), 200 * 128);
}

TEST(RealtimeAllocationTest, Q04_PlaybackOverdubClickAndProbeDoNotAllocate)
{
    SnapshotExchange<ProjectSnapshot> snapshots;
    auto snapshot = std::make_unique<ProjectSnapshot>();
    snapshot->clickBpm = 120.0;
    VoiceSnapshot voice;
    voice.inputDi = 0;
    voice.monitored = true;
    voice.audio = std::make_shared<const TakeAudio>(std::vector<float>(48000, 0.1F));
    snapshot->voices = {voice, voice};
    snapshots.publish(std::move(snapshot));
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
    engine.prepare(48000.0);
    RecordingSession session{{0}};
    recorder.start(session);
    transport.play();
    probe.start(0);
    std::array<float, 128> input{};
    std::array<float, 128> output{};
    const std::array<const float*, 1> inputs{input.data()};
    std::array<float*, 1> outputs{output.data()};
    std::array<float, 128> drained{};

    countAllocations = true;
    for (int block = 0; block < 200; ++block)
    {
        engine.process(inputs.data(), 1, outputs.data(), 1, 128);
        static_cast<void>(session.ring(0).read(drained.data(), drained.size()));
    }
    countAllocations = false;

    EXPECT_EQ(allocationCount, 0);
    EXPECT_EQ(transport.position(), 200 * 128);
}

} // namespace
} // namespace snotes::engine
