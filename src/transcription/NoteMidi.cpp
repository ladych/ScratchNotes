#include "transcription/NoteMidi.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <cmath>

namespace snotes::transcription
{
namespace
{

constexpr double kTicksPerSecond = kMidiTicksPerQuarter * 2.0;
constexpr int kMicrosecondsPerQuarter = 500000;

double ticks(double seconds)
{
    return std::round(seconds * kTicksPerSecond);
}

juce::uint8 velocity(float amplitude)
{
    return static_cast<juce::uint8>(std::clamp(static_cast<int>(std::lround(amplitude * 127.0F)), 1, 127));
}

} // namespace

std::vector<std::uint8_t> notesToMidi(const std::vector<Note>& notes)
{
    juce::MidiMessageSequence track;
    track.addEvent(juce::MidiMessage::tempoMetaEvent(kMicrosecondsPerQuarter), 0.0);
    for (const auto& note : notes)
    {
        track.addEvent(juce::MidiMessage::noteOn(1, note.pitch, velocity(note.amplitude)),
                       ticks(note.startSeconds));
        track.addEvent(juce::MidiMessage::noteOff(1, note.pitch), ticks(note.endSeconds));
    }
    track.updateMatchedPairs();
    juce::MidiFile file;
    file.setTicksPerQuarterNote(kMidiTicksPerQuarter);
    file.addTrack(track);
    juce::MemoryOutputStream stream;
    file.writeTo(stream);
    const auto* const bytes = static_cast<const std::uint8_t*>(stream.getData());
    return {bytes, bytes + stream.getDataSize()};
}

} // namespace snotes::transcription
