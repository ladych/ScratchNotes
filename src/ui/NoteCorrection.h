#pragma once

#include "io/IFileSystem.h"
#include "model/Project.h"
#include "notation/MusicXml.h"
#include "rhythm/NoteEdit.h"

#include <juce_data_structures/juce_data_structures.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{

/// A choice of the menu of a note: a duration in ticks or a string, with its text.
struct NoteChoice
{
    std::int64_t value = 0;
    std::string label;
    bool ticked = false;

    bool operator==(const NoteChoice&) const = default;
};

struct NoteEnvironment
{
    /// Folder of the open project; std::nullopt while it has none.
    std::function<std::optional<std::filesystem::path>()> projectFolder;
    std::function<void(const std::string& title, const std::string& message)> showMessage;
};

/// Presenter for the correction of single notes in the score (F-KO-04, F-TB-03): the selected note can be
/// deleted, moved by semitones, made longer or shorter and played on another string. Every change is an
/// undoable edit of the take (rhythm::NoteEdit); after it, and after its undo, poll() quantizes the voices
/// again (ui::updateRhythm), so the score shows it and keeps it when the grid changes (F-KO-05).
class NoteCorrection final : private juce::ValueTree::Listener
{
public:
    NoteCorrection(juce::ValueTree project,
                   juce::UndoManager& undoManager,
                   io::IFileSystem& fileSystem,
                   NoteEnvironment environment);
    ~NoteCorrection() override;

    NoteCorrection(const NoteCorrection&) = delete;
    NoteCorrection& operator=(const NoteCorrection&) = delete;
    NoteCorrection(NoteCorrection&&) = delete;
    NoteCorrection& operator=(NoteCorrection&&) = delete;

    /// A note of the score; none to select nothing. A note of a part without a take is not selected.
    void select(const std::optional<notation::WrittenNote>& note);
    [[nodiscard]] const std::optional<notation::WrittenNote>& selected() const { return selected_; }

    /// The selection goes with the note.
    void deleteNote();
    /// The note moves by semitones within the MIDI range and is free again to find its string.
    void transpose(int semitones);
    /// The length of the selected note's event, in ticks of the score.
    void setDuration(std::int64_t ticks);
    /// rhythm::NoteEdit::kNone lets the fingering choose again.
    void setString(int string);

    /// Note values from a triplet sixteenth to a whole note.
    [[nodiscard]] std::vector<NoteChoice> durationChoices() const;
    /// The strings the selected note can be played on, the lowest first, and "Automatic"
    /// (rhythm::NoteEdit::kNone) first; the fixed one is ticked.
    [[nodiscard]] std::vector<NoteChoice> stringChoices() const;

    /// Quantizes again if edits changed since; true if it did.
    bool poll();

private:
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;

    /// The selected note with its voice and take and its edit as it stands, found by the pitch it has now;
    /// none if its part is not in the score.
    struct Selection
    {
        model::Voice voice;
        model::Take take;
        notation::WrittenNote note;
        rhythm::NoteEdit edit;
    };
    [[nodiscard]] std::optional<Selection> selection(juce::UndoManager* undoManager) const;
    void store(model::Take take, const rhythm::NoteEdit& edit);

    juce::ValueTree project_;
    juce::UndoManager& undoManager_;
    io::IFileSystem& fileSystem_;
    NoteEnvironment environment_;
    std::optional<notation::WrittenNote> selected_;
    bool changed_ = false;
};

} // namespace snotes::ui
