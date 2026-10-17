#pragma once

#include "engine/ClickTrack.h"
#include "engine/InputMeter.h"
#include "engine/LatencyProbe.h"
#include "engine/PlaybackMixer.h"
#include "engine/ProjectSnapshot.h"
#include "engine/Recorder.h"
#include "engine/SnapshotExchange.h"
#include "engine/TestToneSource.h"
#include "engine/Transport.h"

namespace snotes::engine
{

/// Everything the audio thread does in one block, in a fixed order (F-WI-04): transport, input meters,
/// recording, test tone, playback of the takes, click, monitoring, latency probe. Recording and playback use
/// the same song position of the same block, so an overdub lines up with what played to the sample; the
/// latency is then taken out by where the take is placed (F-WI-05). The audio callback is a thin wrapper
/// around it, so tests can drive whole sessions without a device.
class AudioEngine
{
public:
    struct Parts
    {
        SnapshotExchange<ProjectSnapshot>& snapshots;
        Transport& transport;
        InputMeter& meter;
        Recorder& recorder;
        TestToneSource& tone;
        LatencyProbe& probe;
    };

    explicit AudioEngine(Parts parts) noexcept : parts_(parts) {}

    void prepare(double sampleRate) noexcept;
    void process(const float* const* inputs,
                 int numInputs,
                 float* const* outputs,
                 int numOutputs,
                 int numSamples) noexcept;

private:
    Parts parts_;
    PlaybackMixer mixer_;
    ClickTrack click_;
};

} // namespace snotes::engine
