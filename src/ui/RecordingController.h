#pragma once

#include "engine/Recorder.h"
#include "engine/RecordingSession.h"
#include "io/IFileSystem.h"
#include "io/TakeFileWriter.h"
#include "model/IIdGenerator.h"
#include "model/Project.h"
#include "ui/IWriterThread.h"

#include <juce_data_structures/juce_data_structures.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace snotes::ui
{

/// What the RecordingController needs of the program around it.
struct RecordingEnvironment
{
    using FolderCallback = std::function<void(std::optional<std::filesystem::path>)>;

    /// Reports the project folder, saving a new project first so it has one; std::nullopt if that failed.
    std::function<void(FolderCallback)> projectFolder;
    /// Sample rate of the open audio device, 0 if none is open.
    std::function<double()> deviceSampleRate;
    /// Local date and time for Take::recordedAt, e.g. "2026-10-17T09:40:00".
    std::function<std::string()> now;
    std::function<void(const std::string& title, const std::string& message)> showMessage;
};

/// Presenter for recording (F-AU-02, F-AU-04, F-AU-05): arms voices, starts and stops a pass of all armed
/// voices together and turns it into one take per voice. The DI signal and, if the voice has an input for
/// it, the processed signal go into their own WAV files in the project's audio/ folder.
class RecordingController
{
public:
    enum class State : std::uint8_t
    {
        idle,
        recording,
        /// Stopped, waiting for the audio thread to let go of the session (poll()).
        stopping
    };

    RecordingController(juce::ValueTree project,
                        juce::UndoManager& undoManager,
                        engine::IRecorderControl& recorder,
                        io::IFileSystem& fileSystem,
                        IWriterThread& writerThread,
                        model::IIdGenerator& idGenerator,
                        RecordingEnvironment environment);
    ~RecordingController();

    RecordingController(const RecordingController&) = delete;
    RecordingController& operator=(const RecordingController&) = delete;
    RecordingController(RecordingController&&) = delete;
    RecordingController& operator=(RecordingController&&) = delete;

    void setArmed(const std::string& voiceId, bool armed);
    [[nodiscard]] bool isArmed(const std::string& voiceId) const;

    /// The Rec button: starts if idle, stops while recording.
    void toggle();
    void start();
    void stop();
    /// GUI timer: completes a stopped pass once the audio thread released it.
    void poll();

    [[nodiscard]] State state() const noexcept { return state_; }
    /// Length of the running pass.
    [[nodiscard]] double recordedSeconds() const;

private:
    struct PendingTake
    {
        std::string voiceId;
        model::TakeData data;
    };

    struct TakeFiles
    {
        std::vector<int> inputs;
        std::vector<std::unique_ptr<io::WavWriter>> writers;
        std::vector<PendingTake> pending;
    };

    void begin(const std::filesystem::path& folder, double sampleRate);
    [[nodiscard]] bool createTakeFiles(const model::Voice& voice,
                                       const std::filesystem::path& folder,
                                       double sampleRate,
                                       const std::string& recordedAt,
                                       TakeFiles& files);
    [[nodiscard]] std::vector<model::Voice> armedVoices() const;
    [[nodiscard]] bool hasTakes() const;
    [[nodiscard]] std::string fileBaseName(const model::Voice& voice,
                                           const std::filesystem::path& folder) const;
    void finish();
    void showError(const std::string& message) const;

    juce::ValueTree project_;
    juce::UndoManager& undoManager_;
    engine::IRecorderControl& recorder_;
    io::IFileSystem& fileSystem_;
    IWriterThread& writerThread_;
    model::IIdGenerator& idGenerator_;
    RecordingEnvironment environment_;
    std::set<std::string> armed_;
    State state_ = State::idle;
    double sampleRate_ = 0.0;
    std::unique_ptr<engine::RecordingSession> session_;
    std::unique_ptr<io::TakeFileWriter> writer_;
    std::vector<PendingTake> pending_;
};

} // namespace snotes::ui
