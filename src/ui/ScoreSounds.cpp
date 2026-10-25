#include "ui/ScoreSounds.h"

#include "model/Project.h"
#include "ui/RhythmUpdate.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace snotes::ui
{
namespace
{

/// The notes of a track, nullptr if it has none, and its name.
model::ScoreVoiceNotes trackVoice(const juce::MidiMessageSequence& track, double sampleRate)
{
    model::ScoreVoiceNotes voice;
    auto notes = std::make_shared<engine::SynthNotes>();
    for (const auto* event : track)
    {
        const auto& message = event->message;
        if (message.isTrackNameEvent())
            voice.name = message.getTextFromTextMetaEvent().toStdString();
        // Only note-ons with an end have a note-off.
        if (event->noteOffObject == nullptr)
            continue;
        const engine::SynthNote note{
            .start = std::llround(message.getTimeStamp() * sampleRate),
            .end = std::llround(event->noteOffObject->message.getTimeStamp() * sampleRate),
            .frequency = juce::MidiMessage::getMidiNoteInHertz(message.getNoteNumber())};
        notes->longest = std::max(notes->longest, note.end - note.start);
        notes->notes.push_back(note);
    }
    if (!notes->notes.empty())
        voice.notes = std::move(notes);
    return voice;
}

} // namespace

std::vector<model::ScoreVoiceNotes> midiVoices(const std::string& bytes, double sampleRate)
{
    std::vector<model::ScoreVoiceNotes> voices;
    juce::MemoryInputStream stream{bytes.data(), bytes.size(), false};
    juce::MidiFile file;
    if (!file.readFrom(stream))
        return voices;
    file.convertTimestampTicksToSeconds();
    for (int index = 0; index < file.getNumTracks(); ++index)
    {
        auto voice = trackVoice(*file.getTrack(index), sampleRate);
        if (voice.notes)
            voices.push_back(std::move(voice));
    }
    return voices;
}

ScoreSounds::ScoreSounds(const io::IFileSystem& fileSystem, FolderSource folder)
    : fileSystem_(fileSystem), folder_(std::move(folder))
{
}

std::vector<model::ScoreVoiceNotes> ScoreSounds::voices(const juce::ValueTree& project)
{
    const model::Project model{project, nullptr};
    const auto folder = folder_();
    const bool scored = !model.beatGrid().scoreFile().empty();
    if (!folder || !scored)
        return {};
    auto bytes = fileSystem_.readText(*folder / kMidiFile);
    if (!bytes)
        return {};
    if (*bytes != bytes_ || model.sampleRate() != sampleRate_)
    {
        voices_ = midiVoices(*bytes, model.sampleRate());
        bytes_ = std::move(*bytes);
        sampleRate_ = model.sampleRate();
    }
    return voices_;
}

} // namespace snotes::ui
