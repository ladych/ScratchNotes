#include "app/TranscriptionThread.h"

#include "engine/TakeAudio.h"

#include "rhythm/OnsetStrength.h"
#include "transcription/OnnxPitchModel.h"
#include "transcription/Resampler.h"
#include "transcription/Transcriber.h"

#include <exception>
#include <utility>
#include <vector>

namespace snotes::app
{
namespace
{

constexpr int kStopTimeoutMs = 10000;

std::vector<float> allSamples(const engine::TakeAudio& audio)
{
    std::vector<float> samples(static_cast<std::size_t>(audio.length()));
    audio.read(0, static_cast<int>(audio.length()), samples.data());
    return samples;
}

} // namespace

TranscriptionThread::TranscriptionThread(const void* modelData, std::size_t modelSize)
    : Thread("Scratch Notes transcription"), modelData_(modelData), modelSize_(modelSize)
{
}

TranscriptionThread::~TranscriptionThread()
{
    cancelled_ = true;
    stopThread(kStopTimeoutMs);
}

void TranscriptionThread::start(ui::TranscriptionJob job)
{
    // The thread of the last take may still be returning from run() after it handed over its outcome.
    waitForThreadToExit(kStopTimeoutMs);
    {
        const std::scoped_lock lock{mutex_};
        job_ = std::move(job);
        outcome_.reset();
    }
    cancelled_ = false;
    progress_ = 0.0;
    startThread(Priority::low);
}

std::optional<ui::TranscriptionOutcome> TranscriptionThread::takeOutcome()
{
    const std::scoped_lock lock{mutex_};
    return std::exchange(outcome_, std::nullopt);
}

void TranscriptionThread::run()
{
    std::optional<ui::TranscriptionJob> job;
    {
        const std::scoped_lock lock{mutex_};
        job = std::exchange(job_, std::nullopt);
    }
    if (!job)
        return;
    auto outcome = transcribe(*job);
    const std::scoped_lock lock{mutex_};
    outcome_ = std::move(outcome);
}

ui::TranscriptionOutcome TranscriptionThread::transcribe(const ui::TranscriptionJob& job)
{
    using Status = ui::TranscriptionOutcome::Status;
    try
    {
        const auto audio = loader_.load(job.diFile);
        if (audio == nullptr)
            return {.status = Status::failed, .notes = {}, .onsets = {}};
        const auto samples = allSamples(*audio);
        if (model_ == nullptr)
            model_ = std::make_unique<transcription::OnnxPitchModel>(modelData_, modelSize_);
        auto notes = transcription::transcribe(samples,
                                               job.sampleRate,
                                               *model_,
                                               job.settings,
                                               [this](double done)
                                               {
                                                   progress_ = done;
                                                   return !cancelled_ && !threadShouldExit();
                                               });
        if (!notes)
            return {.status = Status::cancelled, .notes = {}, .onsets = {}};
        return {.status = Status::done,
                .notes = std::move(*notes),
                .onsets = rhythm::measureOnsets(
                    transcription::resample(samples, job.sampleRate, rhythm::kAnalysisRate))};
    }
    catch (const std::exception&)
    {
        return {.status = Status::failed, .notes = {}, .onsets = {}};
    }
}

} // namespace snotes::app
