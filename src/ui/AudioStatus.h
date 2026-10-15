#pragma once

#include <string>

namespace snotes::ui
{

/// What the status bar shows about the audio device (Q-01, Q-02).
struct AudioDeviceInfo
{
    bool open = false;
    std::string driver;
    std::string device;
    double sampleRate = 0.0;
    int bufferSize = 0;
    int inputLatencySamples = 0;  ///< as reported by the driver, 0 if unknown
    int outputLatencySamples = 0; ///< as reported by the driver, 0 if unknown
    int inputChannels = 0;        ///< active input channels
    int outputChannels = 0;       ///< active output channels
    int xruns = -1;       ///< dropouts since the device opened, -1 if the driver does not count them (Q-03)
    bool stalled = false; ///< the device gives no audio, a reopen is under way (Q-09)
    std::string replacedDevice; ///< "type: device" the open fallback stands in for, empty if none (F-AO-03)
};

/// Input latency in ms: the driver's value, or one buffer if the driver reports none.
[[nodiscard]] double inputLatencyMs(const AudioDeviceInfo& info);
/// Output latency in ms: the driver's value, or one buffer if the driver reports none.
[[nodiscard]] double outputLatencyMs(const AudioDeviceInfo& info);
/// Time from the instrument at the input to the sound at the output in ms, as heard while recording an
/// overdub with software monitoring (Q-01). Measure it with a loopback cable (docs/Latenzmessung.md).
[[nodiscard]] double roundTripLatencyMs(const AudioDeviceInfo& info);
/// Status line, e.g. "JACK | system | 48000 Hz | Buffer 128 (2,7 ms) | Roundtrip 8,0 ms (Eingang 2,7 ms,
/// Ausgang 5,3 ms) | 4 Eingänge, 2 Ausgänge | XRuns: 0", marked when the round trip is above the 10 ms of
/// Q-01, with a warning in front while the device hangs or a fallback replaces the chosen device.
[[nodiscard]] std::string audioStatusText(const AudioDeviceInfo& info);

} // namespace snotes::ui
