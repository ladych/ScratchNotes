#include "ui/RecordingController.h"

#include "i18n/Translate.h"
#include "io/Utf8Path.h"
#include "model/ModelIds.h"

#include <utility>

namespace snotes::ui
{
namespace
{

/// "audio/<base> DI.wav" or "audio/<base> processed.wav", relative to the project folder.
std::string audioFile(const std::string& base, bool processed)
{
    std::string suffix = " DI";
    if (processed)
        suffix = i18n::tr(" processed");
    return "audio/" + base + suffix + ".wav";
}

} // namespace

RecordingController::RecordingController(juce::ValueTree project,
                                         juce::UndoManager& undoManager,
                                         engine::IRecorderControl& recorder,
                                         io::IFileSystem& fileSystem,
                                         IWriterThread& writerThread,
                                         model::IIdGenerator& idGenerator,
                                         RecordingEnvironment environment)
    : project_(std::move(project)), undoManager_(undoManager), recorder_(recorder), fileSystem_(fileSystem),
      writerThread_(writerThread), idGenerator_(idGenerator), environment_(std::move(environment))
{
}

RecordingController::~RecordingController()
{
    // Closing the program while recording: the audio written so far stays in the files.
    if (state_ == State::idle)
        return;
    recorder_.stop();
    writerThread_.detach();
    static_cast<void>(writer_->finish());
}

void RecordingController::setArmed(const std::string& voiceId, bool armed)
{
    if (armed)
        armed_.insert(voiceId);
    else
        armed_.erase(voiceId);
}

bool RecordingController::isArmed(const std::string& voiceId) const
{
    return armed_.contains(voiceId);
}

void RecordingController::toggle()
{
    if (state_ == State::idle)
        start();
    else
        stop();
}

void RecordingController::start()
{
    if (state_ != State::idle)
        return;
    if (armedVoices().empty())
    {
        showError(i18n::tr("Arm at least one voice with a DI input (R) first."));
        return;
    }
    const double sampleRate = environment_.deviceSampleRate();
    if (sampleRate <= 0.0)
    {
        showError(i18n::tr("No audio device is open."));
        return;
    }
    const model::Project project{project_, nullptr};
    if (hasTakes() && sampleRate != project.sampleRate())
    {
        showError(i18n::tr("The audio device runs at %1 Hz, but the takes of this project have %2 Hz. Please "
                           "set the device to %2 Hz.",
                           std::to_string(static_cast<int>(sampleRate)),
                           std::to_string(static_cast<int>(project.sampleRate()))));
        return;
    }
    environment_.projectFolder(
        [this, sampleRate](const std::optional<std::filesystem::path>& folder)
        {
            if (folder)
                begin(*folder, sampleRate);
        });
}

void RecordingController::stop()
{
    if (state_ != State::recording)
        return;
    recorder_.stop();
    state_ = State::stopping;
    poll();
}

void RecordingController::poll()
{
    if (state_ == State::stopping && recorder_.isReleased(*session_))
        finish();
}

double RecordingController::recordedSeconds() const
{
    if (!session_)
        return 0.0;
    return static_cast<double>(session_->recordedSamples()) / sampleRate_;
}

void RecordingController::begin(const std::filesystem::path& folder, double sampleRate)
{
    if (!fileSystem_.createDirectories(folder / "audio"))
    {
        showError(i18n::tr("The folder for the recordings could not be created in \"%1\".",
                           io::utf8FromPath(folder)));
        return;
    }

    TakeFiles files;
    const auto recordedAt = environment_.now();
    for (const auto& voice : armedVoices())
        if (!createTakeFiles(voice, folder, sampleRate, recordedAt, files))
            return;

    if (!hasTakes())
        project_.setProperty(model::ids::sampleRate, sampleRate, nullptr);
    sampleRate_ = sampleRate;
    session_ = std::make_unique<engine::RecordingSession>(files.inputs);
    writer_ = std::make_unique<io::TakeFileWriter>(*session_, std::move(files.writers));
    pending_ = std::move(files.pending);
    writerThread_.attach(*writer_);
    recorder_.start(*session_);
    state_ = State::recording;
}

bool RecordingController::createTakeFiles(const model::Voice& voice,
                                          const std::filesystem::path& folder,
                                          double sampleRate,
                                          const std::string& recordedAt,
                                          TakeFiles& files)
{
    const auto base = fileBaseName(voice, folder);
    model::TakeData take;
    take.id = idGenerator_.next();
    take.recordedAt = recordedAt;
    for (const bool processed : {false, true})
    {
        const int input = processed ? voice.inputWet() : voice.inputDi();
        if (input == model::kNoInput)
            continue;
        const auto relative = audioFile(base, processed);
        auto file = fileSystem_.createBinaryFile(folder / io::pathFromUtf8(relative));
        if (!file)
        {
            showError(i18n::tr("\"%1\" could not be created.", relative));
            return false;
        }
        files.inputs.push_back(input);
        files.writers.push_back(std::make_unique<io::WavWriter>(std::move(file), sampleRate));
        (processed ? take.wetFile : take.diFile) = relative;
    }
    PendingTake pending;
    pending.voiceId = voice.id();
    pending.data = std::move(take);
    files.pending.push_back(std::move(pending));
    return true;
}

std::vector<model::Voice> RecordingController::armedVoices() const
{
    std::vector<model::Voice> voices;
    const model::Project project{project_, &undoManager_};
    for (int index = 0; index < project.numVoices(); ++index)
    {
        const auto voice = project.voice(index);
        const auto id = voice.id();
        if (armed_.contains(id) && voice.inputDi() != model::kNoInput)
            voices.push_back(voice);
    }
    return voices;
}

bool RecordingController::hasTakes() const
{
    const model::Project project{project_, nullptr};
    for (int index = 0; index < project.numVoices(); ++index)
        if (project.voice(index).numTakes() > 0)
            return true;
    return false;
}

std::string RecordingController::fileBaseName(const model::Voice& voice,
                                              const std::filesystem::path& folder) const
{
    const auto name =
        juce::File::createLegalFileName(juce::String::fromUTF8(voice.name().c_str())).toStdString();
    for (int number = voice.numTakes() + 1;; ++number)
    {
        auto base = i18n::tr("%1 take %2", name, std::to_string(number));
        const auto di = folder / io::pathFromUtf8(audioFile(base, false));
        const auto processed = folder / io::pathFromUtf8(audioFile(base, true));
        if (!fileSystem_.exists(di) && !fileSystem_.exists(processed))
            return base;
    }
}

void RecordingController::finish()
{
    writerThread_.detach();
    const bool written = writer_->finish();
    const auto dropped = session_->droppedSamples();
    undoManager_.beginNewTransaction();
    for (const auto& take : pending_)
        if (auto voice = model::Project{project_, &undoManager_}.findVoice(take.voiceId))
            voice->addTake(take.data);
    writer_.reset();
    session_.reset();
    pending_.clear();
    state_ = State::idle;
    if (!written)
        showError(i18n::tr("Writing the recording failed. The takes may be incomplete; is the disk full?"));
    else if (dropped > 0)
        showError(i18n::tr("The computer could not write the recording fast enough; %1 samples are missing.",
                           std::to_string(dropped)));
}

void RecordingController::showError(const std::string& message) const
{
    environment_.showMessage(i18n::tr("Recording"), message);
}

} // namespace snotes::ui
