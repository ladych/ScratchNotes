#pragma once

#include "io/AudioFileReader.h"
#include "io/IFileSystem.h"
#include "model/IIdGenerator.h"
#include "model/Project.h"
#include "ui/IDocumentView.h"

#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{

/// What the TakeImport needs of the program around it.
struct TakeImportEnvironment
{
    using FolderCallback = std::function<void(std::optional<std::filesystem::path>)>;

    /// Reports the project folder, saving a new project first so it has one; std::nullopt if that failed.
    std::function<void(FolderCallback)> projectFolder;
    /// True while a pass is recorded; then nothing is imported.
    std::function<bool()> isRecording;
    /// Inputs of the open device, for the inputs of a new voice.
    std::function<int()> numInputs;
    /// Local date and time for Take::recordedAt, e.g. "2026-10-17T09:40:00".
    std::function<std::string()> now;
    std::function<void(const std::string& title, const std::string& message)> showMessage;
    /// Id of the imported take, for the transcription in the background (F-TX-01).
    std::function<void(const std::vector<std::string>& takeIds)> takesImported;
};

/// Which of the chosen files is the DI signal and which the processed one.
struct ImportFiles
{
    std::filesystem::path di;
    /// Empty if only one file was chosen.
    std::filesystem::path processed;
};

/// Assigns one or two chosen files (F-PJ-05): one file is the DI signal. Of two files, the one whose name
/// holds the word "DI" or "dry" is the DI signal, else the other one if its name holds "processed",
/// "bearbeitet", "wet" or "FX"; without such a word the first file is the DI signal. Case does not matter.
[[nodiscard]] ImportFiles assignImportFiles(const std::filesystem::path& first,
                                            const std::filesystem::path& second);

/// The name of a voice made from the file: its name without extension and the words that mark the signal,
/// e.g. "Stimme 1 Take 2" for "Stimme 1 Take 2 DI.wav".
[[nodiscard]] std::string voiceNameFromFile(const std::filesystem::path& file);

/// Mixes all channels to one (their mean) and converts it to `sampleRate`.
[[nodiscard]] std::vector<float> monoAtRate(const io::AudioFileContent& content, double sampleRate);

/// Imports audio files as a take of a voice (F-PJ-05). The files are copied into the project's audio/
/// folder as mono WAV files with 32-bit float samples, named like recorded takes; stereo files are mixed to
/// mono, files with another sample rate than the project's are converted. A project without takes takes
/// the sample rate of the DI file. The imported take starts at the beginning of the song without latency
/// compensation, becomes the active take, is one undo step and is transcribed like a recording.
class TakeImport
{
public:
    TakeImport(juce::ValueTree project,
               juce::UndoManager& undoManager,
               io::IFileSystem& fileSystem,
               io::IAudioFileReader& reader,
               model::IIdGenerator& idGenerator,
               IDocumentView& view,
               TakeImportEnvironment environment);

    /// Asks for one or two files and imports them as a new take of the voice.
    void importInto(const std::string& voiceId);
    /// Asks for one or two files and imports them as the first take of a new voice named after them.
    void importAsNewVoice();

private:
    struct Signal
    {
        io::AudioFileContent content;
        bool processed = false;
    };

    void chooseFiles(std::function<void(const ImportFiles&)> onChosen);
    [[nodiscard]] std::optional<std::vector<Signal>> read(const ImportFiles& files);
    /// A take for the voice `voiceId`, or for a new voice `newVoiceName` without one.
    struct Request
    {
        std::optional<std::string> voiceId;
        std::string newVoiceName;
        std::vector<Signal> signals;
    };

    void importSignals(Request request);
    void write(const std::filesystem::path& folder, const Request& request);
    /// Writes the files of the take and enters them into `take`; false after a failure, which is shown.
    [[nodiscard]] bool writeFiles(const std::filesystem::path& folder,
                                  const std::string& base,
                                  double sampleRate,
                                  const std::vector<Signal>& signals,
                                  model::TakeData& take);
    [[nodiscard]] std::string baseName(const std::optional<model::Voice>& voice,
                                       const std::string& newVoiceName,
                                       const std::filesystem::path& folder) const;
    /// Adds the take to `voice`, or to a new voice `newVoiceName` without one, as one undo step.
    void addToProject(model::Project& project,
                      std::optional<model::Voice> voice,
                      const std::string& newVoiceName,
                      const model::TakeData& take);
    [[nodiscard]] bool hasTakes() const;
    void showError(const std::string& message) const;

    juce::ValueTree project_;
    juce::UndoManager& undoManager_;
    io::IFileSystem& fileSystem_;
    io::IAudioFileReader& reader_;
    model::IIdGenerator& idGenerator_;
    IDocumentView& view_;
    TakeImportEnvironment environment_;
    /// Folder of the last imported file, where the next file dialog starts.
    std::filesystem::path lastFolder_;
};

} // namespace snotes::ui
