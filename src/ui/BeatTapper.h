#pragma once

#include "ui/GridCorrection.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace snotes::ui
{

/// What the BeatTapper needs of the program around it.
struct TapEnvironment
{
    /// Song position of the transport in seconds.
    std::function<double()> positionSeconds;
    std::function<bool()> isPlaying;
    /// How long the sound takes from the transport to the speakers.
    std::function<double()> outputLatencySeconds;
};

/// Presenter for tapping the beat along (F-KO-03): every tap during playback is a beat where the sound was
/// heard; when the playback stops, the taps replace the beats of their stretch
/// (GridCorrection::replaceBeats).
class BeatTapper
{
public:
    BeatTapper(GridCorrection& correction, TapEnvironment environment);

    /// The key for tapping; nothing while the transport stands still.
    void tap();
    /// Called regularly; hands the taps over once the playback has stopped.
    void poll();
    /// E.g. "Tapping the beat: 5 beats", empty while nobody taps.
    [[nodiscard]] std::string statusText() const;

private:
    GridCorrection& correction_;
    TapEnvironment environment_;
    std::vector<double> taps_;
};

} // namespace snotes::ui
