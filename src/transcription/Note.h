#pragma once

namespace snotes::transcription
{

/// A detected note in the time of the recording.
struct Note
{
    double startSeconds = 0.0;
    double endSeconds = 0.0;
    /// MIDI note.
    int pitch = 0;
    /// Mean note activation of the model, 0..1.
    float amplitude = 0.0F;

    bool operator==(const Note&) const = default;
};

} // namespace snotes::transcription
