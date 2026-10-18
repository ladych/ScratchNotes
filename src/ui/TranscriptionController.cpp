#include "ui/TranscriptionController.h"

#include "i18n/Translate.h"
#include "io/Utf8Path.h"
#include "model/InstrumentProfile.h"
#include "transcription/NoteCsv.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace snotes::ui
{

std::string notesFileFor(const std::string& diFile)
{
    auto file = io::pathFromUtf8(diFile);
    file.replace_extension(".notes.csv");
    return io::genericUtf8FromPath(file);
}

TranscriptionController::TranscriptionController(juce::ValueTree project,
                                                 io::IFileSystem& fileSystem,
                                                 ITranscriptionWorker& worker,
                                                 TranscriptionEnvironment environment)
    : project_(std::move(project)), fileSystem_(fileSystem), worker_(worker),
      environment_(std::move(environment))
{
}

void TranscriptionController::transcribeNew(const std::vector<std::string>& takeIds)
{
    for (const auto& id : takeIds)
    {
        const auto located = locate(id);
        if (!located)
            continue;
        const auto take = takeAt(*located);
        if (!take.hasNotes())
            enqueue(id);
    }
    startNext();
}

void TranscriptionController::transcribeActiveTakes()
{
    const model::Project project{project_, nullptr};
    for (int index = 0; index < project.numVoices(); ++index)
        if (const auto take = project.voice(index).activeTake())
            enqueue(take->id());
    startNext();
}

void TranscriptionController::cancel()
{
    queue_.clear();
    if (running_)
        worker_.cancel();
}

void TranscriptionController::poll()
{
    if (!running_)
        return;
    const auto outcome = worker_.takeOutcome();
    if (!outcome)
        return;
    const auto takeId = running_.value();
    running_.reset();
    if (outcome->status == TranscriptionOutcome::Status::done)
        store(takeId, outcome->notes);
    else if (outcome->status == TranscriptionOutcome::Status::failed)
        environment_.showMessage(
            i18n::tr("Note detection"),
            i18n::tr("The notes of a take could not be detected; is its DI file readable?"));
    startNext();
}

std::string TranscriptionController::statusText() const
{
    if (!running_)
        return {};
    auto text = runningText(*running_, std::to_string(std::lround(worker_.progress() * 100.0)));
    if (!queue_.empty())
        text += i18n::tr(" (%1 more)", std::to_string(queue_.size()));
    return text;
}

std::string TranscriptionController::runningText(const std::string& takeId, const std::string& percent) const
{
    const auto located = locate(takeId);
    if (!located)
        return i18n::tr("Detecting notes: %1 %", percent);
    const auto name = voiceAt(*located).name();
    const auto number = std::to_string(located->take + 1);
    return i18n::tr("Detecting notes: %1, take %2, %3 %", name, number, percent);
}

void TranscriptionController::enqueue(const std::string& takeId)
{
    if (running_ != takeId && std::ranges::find(queue_, takeId) == queue_.end())
        queue_.push_back(takeId);
}

void TranscriptionController::startNext()
{
    while (!running_ && !queue_.empty())
    {
        const auto takeId = queue_.front();
        queue_.pop_front();
        if (auto job = jobFor(takeId))
        {
            running_ = takeId;
            worker_.start(std::move(*job));
        }
    }
}

std::optional<TranscriptionJob> TranscriptionController::jobFor(const std::string& takeId) const
{
    const auto folder = environment_.projectFolder();
    if (!folder)
        return std::nullopt;
    const auto located = locate(takeId);
    if (!located)
        return std::nullopt;
    const auto take = takeAt(*located);
    if (take.isAudioMissing())
        return std::nullopt;
    const auto profile = model::findInstrumentProfile(voiceAt(*located).profileId())
                             .value_or(model::instrumentProfiles().front());
    TranscriptionJob job;
    job.diFile = *folder;
    job.diFile /= io::pathFromUtf8(take.diFile());
    job.sampleRate = model::Project{project_, nullptr}.sampleRate();
    job.settings = transcription::settingsFor(profile);
    return job;
}

void TranscriptionController::store(const std::string& takeId, const std::vector<transcription::Note>& notes)
{
    const auto folder = environment_.projectFolder();
    const auto located = locate(takeId);
    // The project was closed or the take deleted while it was transcribed.
    if (!folder || !located)
        return;
    auto take = takeAt(*located);
    const auto file = notesFileFor(take.diFile());
    if (!fileSystem_.writeText(*folder / io::pathFromUtf8(file), transcription::notesToCsv(notes)))
    {
        environment_.showMessage(i18n::tr("Note detection"),
                                 i18n::tr("Writing the notes failed; is the disk full?"));
        return;
    }
    take.setNotes(file, static_cast<int>(notes.size()));
}

std::optional<TranscriptionController::Located>
TranscriptionController::locate(const std::string& takeId) const
{
    const model::Project project{project_, nullptr};
    for (int voice = 0; voice < project.numVoices(); ++voice)
        if (const auto index = project.voice(voice).takeIndex(takeId); index >= 0)
            return Located{.voice = voice, .take = index};
    return std::nullopt;
}

model::Voice TranscriptionController::voiceAt(Located located) const
{
    return model::Project{project_, nullptr}.voice(located.voice);
}

model::Take TranscriptionController::takeAt(Located located) const
{
    return voiceAt(located).take(located.take);
}

} // namespace snotes::ui
