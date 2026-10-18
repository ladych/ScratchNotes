#pragma once

#include "transcription/Note.h"
#include "transcription/Transcriber.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace snotes::ui
{

/// One take to transcribe (F-TX-01).
struct TranscriptionJob
{
    std::filesystem::path diFile;
    double sampleRate = 0.0;
    transcription::TranscriptionSettings settings;
};

struct TranscriptionOutcome
{
    enum class Status : std::uint8_t
    {
        done,
        cancelled,
        /// The file could not be read or the model failed.
        failed
    };

    Status status = Status::done;
    std::vector<transcription::Note> notes;
};

/// Runs one transcription at a time away from the GUI thread; the GUI thread asks for progress and result.
class ITranscriptionWorker
{
public:
    virtual ~ITranscriptionWorker() = default;

    /// Only called while no job runs.
    virtual void start(TranscriptionJob job) = 0;
    /// The running job stops soon and finishes as cancelled.
    virtual void cancel() = 0;
    /// Share of the running job that is done, 0 to 1.
    [[nodiscard]] virtual double progress() const = 0;
    /// The outcome of the job once it finished, then nothing until the next one finished.
    [[nodiscard]] virtual std::optional<TranscriptionOutcome> takeOutcome() = 0;

protected:
    ITranscriptionWorker() = default;
    ITranscriptionWorker(const ITranscriptionWorker&) = default;
    ITranscriptionWorker(ITranscriptionWorker&&) = default;
    ITranscriptionWorker& operator=(const ITranscriptionWorker&) = default;
    ITranscriptionWorker& operator=(ITranscriptionWorker&&) = default;
};

} // namespace snotes::ui
