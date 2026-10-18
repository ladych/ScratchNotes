#pragma once

#include "engine/Transport.h"

#include <functional>

namespace snotes::ui
{

/// What the TransportControl needs of the program around it.
struct TransportEnvironment
{
    /// Sample rate of the open device, 0 without one.
    std::function<double()> sampleRate;
    /// True while a recording runs or finishes.
    std::function<bool()> isRecording;
    /// Ends the recording, which also stops the transport.
    std::function<void()> stopRecording;
};

/// Presenter of the transport bar (F-WI-01): play and stop, back to the start, the song position. While
/// recording, Stop ends the recording and the position cannot be moved.
class TransportControl
{
public:
    TransportControl(engine::Transport& transport, TransportEnvironment environment);

    /// The Play/Stop button and the space bar.
    void togglePlay();
    /// Back to the song start.
    void returnToStart();
    /// Moves to a time in the song, e.g. a click into the tracks.
    void locate(double seconds);

    [[nodiscard]] bool isPlaying() const { return transport_.isPlaying(); }
    [[nodiscard]] double positionSeconds() const;

private:
    engine::Transport& transport_;
    TransportEnvironment environment_;
};

} // namespace snotes::ui
