#include "notation/ScoreMidi.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>

namespace snotes::notation
{
namespace
{

constexpr int kMidiTicksPerTick = kMidiTicksPerBeat / static_cast<int>(kTicksPerBeat);
constexpr int kGraceTicks = kMidiTicksPerBeat / 8;
constexpr int kVelocity = 100;
/// General MIDI programs, counted from 0.
constexpr int kGuitarProgram = 27;
constexpr int kBassProgram = 33;
constexpr int kDrumChannel = 10;
constexpr int kChannels = 16;
constexpr double kMicroseconds = 1e6;
constexpr double kLongestQuarter = 0xFFFFFF;
/// A lead-in can not be shorter than this.
constexpr double kShortestLeadIn = 1e-3;

/// beatsPerBar/4 with the usual metronome of 24 MIDI clocks and 8 thirty-seconds per quarter; JUCE's own
/// event writes 1 and 96 there, which some programs take literally.
juce::MidiMessage timeSignature(int beatsPerBar)
{
    constexpr int kQuarterPowerOfTwo = 2;
    constexpr int kClocksPerClick = 24;
    constexpr int kThirtySecondsPerQuarter = 8;
    const std::array<juce::uint8, 7> bytes{0xFF,
                                           0x58,
                                           4,
                                           static_cast<juce::uint8>(beatsPerBar),
                                           kQuarterPowerOfTwo,
                                           kClocksPerClick,
                                           kThirtySecondsPerQuarter};
    return {bytes.data(), static_cast<int>(bytes.size()), 0.0};
}

juce::MidiMessage tempo(double secondsPerBeat)
{
    return juce::MidiMessage::tempoMetaEvent(
        static_cast<int>(std::clamp(std::round(secondsPerBeat * kMicroseconds), 1.0, kLongestQuarter)));
}

/// Channels 1 to 16 without the drum channel 10.
int channelOf(std::size_t voice)
{
    const auto channel = static_cast<int>(voice % (kChannels - 1)) + 1;
    return channel < kDrumChannel ? channel : channel + 1;
}

void addChord(
    juce::MidiMessageSequence& track, int channel, const std::vector<int>& pitches, double start, double end)
{
    for (const int pitch : pitches)
    {
        track.addEvent(juce::MidiMessage::noteOn(channel, pitch, static_cast<juce::uint8>(kVelocity)), start);
        track.addEvent(juce::MidiMessage::noteOff(channel, pitch), end);
    }
}

juce::MidiMessageSequence voiceTrack(const ScoreVoice& voice, std::size_t index, double leadTicks)
{
    const auto channel = channelOf(index);
    juce::MidiMessageSequence track;
    track.addEvent(juce::MidiMessage::textMetaEvent(3, juce::String::fromUTF8(voice.name.c_str())), 0.0);
    track.addEvent(juce::MidiMessage::programChange(
                       channel, voice.clef == model::Clef::bass8vb ? kBassProgram : kGuitarProgram),
                   0.0);
    for (const auto& event : voice.events)
    {
        const auto start = leadTicks + static_cast<double>(event.startTick * kMidiTicksPerTick);
        const auto graces = static_cast<double>(event.graces.size());
        for (std::size_t grace = 0; grace < event.graces.size(); ++grace)
        {
            const auto graceStart = start - ((graces - static_cast<double>(grace)) * kGraceTicks);
            addChord(track,
                     channel,
                     event.graces[grace].pitches,
                     std::max(0.0, graceStart),
                     std::max(0.0, graceStart + kGraceTicks));
        }
        addChord(track,
                 channel,
                 event.chord.pitches,
                 start,
                 start + static_cast<double>(event.durationTicks * kMidiTicksPerTick));
    }
    track.updateMatchedPairs();
    return track;
}

} // namespace

double secondsAt(std::span<const double> beats, double beat)
{
    const auto last = static_cast<double>(beats.size() - 1);
    const auto index = static_cast<std::size_t>(std::clamp(std::floor(beat), 0.0, last - 1.0));
    return beats[index] + ((beat - static_cast<double>(index)) * (beats[index + 1] - beats[index]));
}

std::vector<std::uint8_t> scoreToMidi(const Score& score, std::span<const double> beats)
{
    const auto origin = static_cast<double>(score.originTick) / static_cast<double>(kTicksPerBeat);
    const auto pickupBeats = static_cast<double>(score.pickupTicks) / static_cast<double>(kTicksPerBeat);
    const auto start = std::max(kShortestLeadIn, secondsAt(beats, origin));
    const auto firstBeat = secondsAt(beats, origin + 1.0) - secondsAt(beats, origin);
    const auto leadBars = std::max(1.0, std::round(((start / firstBeat) + pickupBeats) / score.beatsPerBar));
    const auto leadBeats = (leadBars * score.beatsPerBar) - pickupBeats;
    const auto leadTicks = leadBeats * kMidiTicksPerBeat;

    juce::MidiMessageSequence tempoTrack;
    tempoTrack.addEvent(timeSignature(score.beatsPerBar), 0.0);
    tempoTrack.addEvent(tempo(start / leadBeats), 0.0);
    const auto scoreTicks = score.pickupTicks + (score.bars * score.barTicks());
    for (std::int64_t beat = 0; beat * kTicksPerBeat < scoreTicks; ++beat)
    {
        const auto position = origin + static_cast<double>(beat);
        tempoTrack.addEvent(tempo(secondsAt(beats, position + 1.0) - secondsAt(beats, position)),
                            leadTicks + static_cast<double>(beat * kMidiTicksPerBeat));
    }

    juce::MidiFile file;
    file.setTicksPerQuarterNote(kMidiTicksPerBeat);
    file.addTrack(tempoTrack);
    for (std::size_t voice = 0; voice < score.voices.size(); ++voice)
        file.addTrack(voiceTrack(score.voices[voice], voice, leadTicks));
    juce::MemoryOutputStream stream;
    file.writeTo(stream);
    const auto* const bytes = static_cast<const std::uint8_t*>(stream.getData());
    return {bytes, bytes + stream.getDataSize()};
}

} // namespace snotes::notation
