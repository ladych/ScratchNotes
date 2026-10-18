#pragma once

#include "io/IFileSystem.h"
#include "model/Project.h"
#include "ui/ITranscriptionWorker.h"

#include <juce_data_structures/juce_data_structures.h>

#include <deque>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{

/// What the TranscriptionController needs of the program around it.
struct TranscriptionEnvironment
{
    /// Folder of the open project; std::nullopt while it has none.
    std::function<std::optional<std::filesystem::path>()> projectFolder;
    std::function<void(const std::string& title, const std::string& message)> showMessage;
};

/// Presenter for the transcription in the background (F-TX-01): takes are queued and handed to the worker
/// one after the other, after recording or by command. The notes of a take go next to its DI file as
/// "<DI file name>.notes.csv"; the take remembers the file and the number of notes.
class TranscriptionController
{
public:
    TranscriptionController(juce::ValueTree project,
                            io::IFileSystem& fileSystem,
                            ITranscriptionWorker& worker,
                            TranscriptionEnvironment environment);

    /// After recording: the new takes that have no notes yet.
    void transcribeNew(const std::vector<std::string>& takeIds);
    /// Command: the active take of every voice, also if it has notes already.
    void transcribeActiveTakes();
    /// Stops the running take and forgets the queued ones.
    void cancel();
    /// GUI timer: stores a finished take and starts the next one.
    void poll();

    [[nodiscard]] bool isRunning() const { return running_.has_value(); }
    /// E.g. "Detecting notes: Bass, take 2, 40 %", empty while nothing runs.
    [[nodiscard]] std::string statusText() const;

private:
    struct Located
    {
        int voice;
        int take;
    };

    void enqueue(const std::string& takeId);
    void startNext();
    void store(const std::string& takeId, const std::vector<transcription::Note>& notes);
    [[nodiscard]] std::optional<TranscriptionJob> jobFor(const std::string& takeId) const;
    [[nodiscard]] std::optional<Located> locate(const std::string& takeId) const;
    [[nodiscard]] model::Voice voiceAt(Located located) const;
    [[nodiscard]] model::Take takeAt(Located located) const;
    [[nodiscard]] std::string runningText(const std::string& takeId, const std::string& percent) const;

    juce::ValueTree project_;
    io::IFileSystem& fileSystem_;
    ITranscriptionWorker& worker_;
    TranscriptionEnvironment environment_;
    std::deque<std::string> queue_;
    std::optional<std::string> running_;
};

/// Notes file of a DI file: "audio/Bass Take 1 DI.wav" becomes "audio/Bass Take 1 DI.notes.csv".
[[nodiscard]] std::string notesFileFor(const std::string& diFile);

} // namespace snotes::ui
