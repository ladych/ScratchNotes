#include "transcription/Transcriber.h"

#include "transcription/Resampler.h"

#include <algorithm>
#include <tuple>

namespace snotes::transcription
{
namespace
{

constexpr int kOctave = 12;

std::vector<Note> inSeconds(const std::vector<FrameNote>& frameNotes)
{
    std::vector<Note> notes;
    notes.reserve(frameNotes.size());
    for (const auto& note : frameNotes)
        notes.push_back({.startSeconds = frameToSeconds(note.startFrame),
                         .endSeconds = frameToSeconds(note.endFrame),
                         .pitch = note.pitch,
                         .amplitude = note.amplitude});
    return notes;
}

/// Undoes the octave shift: the audio played at half speed, so times double and pitches drop an octave.
void octaveDown(std::vector<Note>& notes)
{
    for (auto& note : notes)
    {
        note.startSeconds *= 2.0;
        note.endSeconds *= 2.0;
        note.pitch -= kOctave;
    }
}

} // namespace

TranscriptionSettings settingsFor(const model::InstrumentProfile& profile)
{
    TranscriptionSettings settings;
    settings.octaveUp = profile.transcribeOctaveUp;
    settings.lowestPitch = profile.strings.front();
    settings.highestPitch = profile.strings.back() + profile.frets;
    return settings;
}

std::optional<std::vector<Note>> transcribe(std::span<const float> audio,
                                            double sampleRate,
                                            IPitchModel& model,
                                            const TranscriptionSettings& settings,
                                            const ProgressCallback& progress)
{
    // Resampled to half the model rate, the audio sounds an octave higher when the model reads it at its
    // rate.
    const double modelRate = settings.octaveUp ? basicpitch::kSampleRate / 2.0 : basicpitch::kSampleRate;
    const auto samples = resample(audio, sampleRate, modelRate);
    const auto activations = analysePitches(samples, model, progress);
    if (!activations)
        return std::nullopt;
    auto notes = applyLevelGate(inSeconds(createNotes(*activations, settings.notes)), samples, settings.gate);
    if (settings.octaveUp)
        octaveDown(notes);
    std::erase_if(notes,
                  [&settings](const Note& note)
                  { return note.pitch < settings.lowestPitch || note.pitch > settings.highestPitch; });
    std::ranges::sort(
        notes,
        [](const Note& left, const Note& right)
        { return std::tie(left.startSeconds, left.pitch) < std::tie(right.startSeconds, right.pitch); });
    return notes;
}

} // namespace snotes::transcription
