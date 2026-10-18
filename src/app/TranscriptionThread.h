#pragma once

#include "app/JuceTakeAudioLoader.h"
#include "transcription/PitchModel.h"
#include "ui/ITranscriptionWorker.h"

#include <juce_core/juce_core.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>

namespace snotes::app
{

/// Humble object (E-03): runs transcription::transcribe for one take on its own thread (F-TX-01), so the
/// GUI and the audio keep running. The Basic Pitch model is loaded with the first take.
class TranscriptionThread final : public ui::ITranscriptionWorker, private juce::Thread
{
public:
    TranscriptionThread(const void* modelData, std::size_t modelSize);
    ~TranscriptionThread() override;

    TranscriptionThread(const TranscriptionThread&) = delete;
    TranscriptionThread& operator=(const TranscriptionThread&) = delete;
    TranscriptionThread(TranscriptionThread&&) = delete;
    TranscriptionThread& operator=(TranscriptionThread&&) = delete;

    void start(ui::TranscriptionJob job) override;
    void cancel() override { cancelled_ = true; }
    [[nodiscard]] double progress() const override { return progress_; }
    [[nodiscard]] std::optional<ui::TranscriptionOutcome> takeOutcome() override;

private:
    void run() override;
    [[nodiscard]] ui::TranscriptionOutcome transcribe(const ui::TranscriptionJob& job);

    const void* modelData_;
    std::size_t modelSize_;
    JuceTakeAudioLoader loader_;
    std::unique_ptr<transcription::IPitchModel> model_;
    std::atomic<bool> cancelled_ = false;
    std::atomic<double> progress_ = 0.0;
    std::mutex mutex_;
    std::optional<ui::TranscriptionJob> job_;
    std::optional<ui::TranscriptionOutcome> outcome_;
};

} // namespace snotes::app
