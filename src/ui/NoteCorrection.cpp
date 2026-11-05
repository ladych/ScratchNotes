#include "ui/NoteCorrection.h"

#include "i18n/Translate.h"
#include "model/InstrumentProfile.h"
#include "model/ModelIds.h"
#include "model/Project.h"
#include "ui/RhythmUpdate.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace snotes::ui
{
namespace
{

constexpr int kHighestPitch = 127;
constexpr int kNone = rhythm::NoteEdit::kNone;

struct Duration
{
    std::int64_t ticks;
    const char* label;
};

constexpr std::array kDurations{Duration{.ticks = 2, .label = i18n::marked("Sixteenth triplet")},
                                Duration{.ticks = 3, .label = i18n::marked("Sixteenth")},
                                Duration{.ticks = 4, .label = i18n::marked("Eighth triplet")},
                                Duration{.ticks = 6, .label = i18n::marked("Eighth")},
                                Duration{.ticks = 8, .label = i18n::marked("Quarter triplet")},
                                Duration{.ticks = 9, .label = i18n::marked("Dotted eighth")},
                                Duration{.ticks = 12, .label = i18n::marked("Quarter")},
                                Duration{.ticks = 18, .label = i18n::marked("Dotted quarter")},
                                Duration{.ticks = 24, .label = i18n::marked("Half")},
                                Duration{.ticks = 36, .label = i18n::marked("Dotted half")},
                                Duration{.ticks = 48, .label = i18n::marked("Whole")}};

/// The part of the score with the take it shows.
struct Part
{
    model::Voice voice;
    model::Take take;
};

/// A part of the score: the voices whose active take has events make the parts, in their order
/// (ui::updateRhythm).
std::optional<Part> partOf(const model::Project& project, int part)
{
    int index = 0;
    for (int voice = 0; voice < project.numVoices(); ++voice)
    {
        auto candidate = project.voice(voice);
        auto take = candidate.activeTake();
        if (!take)
            continue;
        const bool scored = !take->eventsFile().empty();
        if (scored && index++ == part)
            return Part{.voice = std::move(candidate), .take = std::move(*take)};
    }
    return std::nullopt;
}

/// The edit of the note that has the pitch now: one that moved a note of the event there, else the one of
/// the note itself, else a new one.
rhythm::NoteEdit editOf(const std::vector<rhythm::NoteEdit>& edits, double seconds, int pitch)
{
    for (const auto& edit : edits)
    {
        const bool there = edit.toPitch == pitch && !edit.deleted;
        if (there && edit.sameNote({.seconds = seconds, .pitch = edit.pitch}))
            return edit;
    }
    const rhythm::NoteEdit wanted{.seconds = seconds, .pitch = pitch};
    for (const auto& edit : edits)
        if (edit.sameNote(wanted))
            return edit;
    return wanted;
}

} // namespace

NoteCorrection::NoteCorrection(juce::ValueTree project,
                               juce::UndoManager& undoManager,
                               io::IFileSystem& fileSystem,
                               NoteEnvironment environment)
    : project_(std::move(project)), undoManager_(undoManager), fileSystem_(fileSystem),
      environment_(std::move(environment))
{
    project_.addListener(this);
}

NoteCorrection::~NoteCorrection()
{
    project_.removeListener(this);
}

void NoteCorrection::select(const std::optional<notation::WrittenNote>& note)
{
    selected_ = note;
    if (!selection(nullptr))
        selected_.reset();
}

void NoteCorrection::deleteNote()
{
    auto current = selection(&undoManager_);
    if (!current)
        return;
    current->edit.deleted = true;
    store(current->take, current->edit);
    selected_.reset();
}

void NoteCorrection::transpose(int semitones)
{
    auto current = selection(&undoManager_);
    if (!current)
        return;
    auto note = current->note;
    note.pitch = std::clamp(note.pitch + semitones, 0, kHighestPitch);
    current->edit.toPitch = note.pitch == current->edit.pitch ? kNone : note.pitch;
    current->edit.string = kNone;
    store(current->take, current->edit);
    selected_ = note;
}

void NoteCorrection::setDuration(std::int64_t ticks)
{
    const auto current = selection(&undoManager_);
    if (!current)
        return;
    store(current->take, {.seconds = current->note.seconds, .pitch = kNone, .durationTicks = ticks});
}

void NoteCorrection::setString(int string)
{
    auto current = selection(&undoManager_);
    if (!current)
        return;
    current->edit.string = string;
    store(current->take, current->edit);
}

std::vector<NoteChoice> NoteCorrection::durationChoices() const
{
    std::int64_t ticked = 0;
    const auto current = selection(nullptr);
    if (current)
        for (const auto& edit : current->take.noteEdits())
            if (edit.sameNote({.seconds = current->note.seconds, .pitch = kNone}))
                ticked = edit.durationTicks;
    std::vector<NoteChoice> choices;
    choices.reserve(kDurations.size());
    for (const auto& duration : kDurations)
        choices.push_back(
            {.value = duration.ticks, .label = i18n::tr(duration.label), .ticked = duration.ticks == ticked});
    return choices;
}

std::vector<NoteChoice> NoteCorrection::stringChoices() const
{
    const auto current = selection(nullptr);
    if (!current)
        return {};
    const auto profile =
        model::findInstrumentProfile(current->voice.profileId()).value_or(model::defaultInstrumentProfile());
    const auto fixed = current->edit.string;
    std::vector<NoteChoice> choices;
    auto automatic = i18n::tr("Automatic");
    choices.push_back({.value = kNone, .label = std::move(automatic), .ticked = fixed == kNone});
    const auto strings = static_cast<int>(profile.strings.size());
    for (int string = 0; string < strings; ++string)
    {
        const auto fret = current->note.pitch - profile.strings[static_cast<std::size_t>(string)];
        const bool reached = fret >= 0 && fret <= profile.frets;
        if (!reached)
            continue;
        choices.push_back(
            {.value = string,
             .label = i18n::tr("String %1, fret %2", std::to_string(strings - string), std::to_string(fret)),
             .ticked = fixed == string});
    }
    return choices;
}

bool NoteCorrection::poll()
{
    if (!changed_)
        return false;
    changed_ = false;
    const auto folder = environment_.projectFolder();
    if (!folder)
        return false;
    if (updateRhythm(project_, fileSystem_, *folder) == RhythmUpdate::writeFailed)
        environment_.showMessage(i18n::tr("Correct notes"),
                                 i18n::tr("Writing the rhythm failed; is the disk full?"));
    return true;
}

void NoteCorrection::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& /*property*/)
{
    if (tree.hasType(model::ids::noteEdit))
        changed_ = true;
}

void NoteCorrection::valueTreeChildAdded(juce::ValueTree& /*parent*/, juce::ValueTree& child)
{
    if (child.hasType(model::ids::noteEdit))
        changed_ = true;
}

void NoteCorrection::valueTreeChildRemoved(juce::ValueTree& /*parent*/, juce::ValueTree& child, int /*index*/)
{
    if (child.hasType(model::ids::noteEdit))
        changed_ = true;
}

std::optional<NoteCorrection::Selection> NoteCorrection::selection(juce::UndoManager* undoManager) const
{
    if (!selected_)
        return std::nullopt;
    const auto& note = *selected_;
    auto part = partOf(model::Project{project_, undoManager}, note.part);
    if (!part)
        return std::nullopt;
    const auto edits = part->take.noteEdits();
    const auto edit = editOf(edits, note.seconds, note.pitch);
    return Selection{.voice = part->voice, .take = part->take, .note = note, .edit = edit};
}

void NoteCorrection::store(model::Take take, const rhythm::NoteEdit& edit)
{
    undoManager_.beginNewTransaction();
    take.setNoteEdit(edit);
}

} // namespace snotes::ui
