#include "ui/TakeImport.h"

#include "i18n/Translate.h"
#include "io/Utf8Path.h"
#include "io/WavWriter.h"
#include "model/ModelIds.h"
#include "transcription/Resampler.h"
#include "ui/TakeFileNames.h"
#include "ui/VoiceList.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <memory>
#include <string_view>
#include <utility>

namespace snotes::ui
{
namespace
{

constexpr std::array<std::string_view, 2> kDiWords{"di", "dry"};
constexpr std::array<std::string_view, 4> kProcessedWords{"processed", "bearbeitet", "wet", "fx"};

std::string lowered(std::string text)
{
    std::ranges::transform(
        text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

/// The words of the file name without extension, split at spaces, underscores, hyphens and dots.
std::vector<std::string> wordsOf(const std::filesystem::path& file)
{
    std::vector<std::string> words;
    std::string word;
    for (const char c : io::utf8FromPath(file.stem()))
    {
        if (c == ' ' || c == '_' || c == '-' || c == '.')
        {
            if (!word.empty())
                words.push_back(std::exchange(word, {}));
        }
        else
            word.push_back(c);
    }
    if (!word.empty())
        words.push_back(word);
    return words;
}

template <std::size_t N> bool isOneOf(const std::string& word, const std::array<std::string_view, N>& list)
{
    return std::ranges::find(list, lowered(word)) != list.end();
}

template <std::size_t N>
bool hasWord(const std::filesystem::path& file, const std::array<std::string_view, N>& list)
{
    return std::ranges::any_of(wordsOf(file),
                               [&list](const std::string& word) { return isOneOf(word, list); });
}

std::string fileName(const std::filesystem::path& file)
{
    return io::utf8FromPath(file.filename());
}

} // namespace

ImportFiles assignImportFiles(const std::filesystem::path& first, const std::filesystem::path& second)
{
    const bool firstDi = hasWord(first, kDiWords);
    const bool secondDi = hasWord(second, kDiWords);
    ImportFiles files;
    files.di = first;
    files.processed = second;
    if (firstDi != secondDi)
    {
        if (secondDi)
            std::swap(files.di, files.processed);
        return files;
    }
    if (!hasWord(second, kProcessedWords) && hasWord(first, kProcessedWords))
        std::swap(files.di, files.processed);
    return files;
}

std::string voiceNameFromFile(const std::filesystem::path& file)
{
    std::string name;
    for (const auto& word : wordsOf(file))
    {
        if (isOneOf(word, kDiWords) || isOneOf(word, kProcessedWords))
            continue;
        if (!name.empty())
            name += ' ';
        name += word;
    }
    if (name.empty())
        name = io::utf8FromPath(file.stem());
    return name;
}

std::vector<float> monoAtRate(const io::AudioFileContent& content, double sampleRate)
{
    const std::size_t length = content.channels.empty() ? 0 : content.channels.front().size();
    std::vector<float> mono(length, 0.0F);
    const auto weight = 1.0F / static_cast<float>(std::max<std::size_t>(content.channels.size(), 1));
    for (const auto& channel : content.channels)
        for (std::size_t index = 0; index < length; ++index)
            mono[index] += channel[index] * weight;
    if (content.sampleRate == sampleRate)
        return mono;
    return transcription::resample(mono, content.sampleRate, sampleRate);
}

TakeImport::TakeImport(juce::ValueTree project,
                       juce::UndoManager& undoManager,
                       io::IFileSystem& fileSystem,
                       io::IAudioFileReader& reader,
                       model::IIdGenerator& idGenerator,
                       IDocumentView& view,
                       TakeImportEnvironment environment)
    : project_(std::move(project)), undoManager_(undoManager), fileSystem_(fileSystem), reader_(reader),
      idGenerator_(idGenerator), view_(view), environment_(std::move(environment))
{
}

void TakeImport::importInto(const std::string& voiceId)
{
    chooseFiles(
        [this, voiceId](const ImportFiles& files)
        {
            auto signals = read(files);
            if (!signals)
                return;
            Request request;
            request.voiceId = voiceId;
            request.signals = std::move(*signals);
            importSignals(std::move(request));
        });
}

void TakeImport::importAsNewVoice()
{
    chooseFiles(
        [this](const ImportFiles& files)
        {
            auto signals = read(files);
            if (!signals)
                return;
            Request request;
            request.newVoiceName = voiceNameFromFile(files.di);
            request.signals = std::move(*signals);
            importSignals(std::move(request));
        });
}

void TakeImport::chooseFiles(std::function<void(const ImportFiles&)> onChosen)
{
    if (environment_.isRecording())
    {
        showError(i18n::tr("Stop the recording first."));
        return;
    }
    view_.chooseAudioFiles(
        lastFolder_,
        [this, onChosen = std::move(onChosen)](const std::vector<std::filesystem::path>& files)
        {
            if (files.empty())
                return;
            if (files.size() > 2)
            {
                showError(i18n::tr("Choose one file, or two: the DI and the processed signal."));
                return;
            }
            lastFolder_ = files.front().parent_path();
            ImportFiles chosen;
            chosen.di = files.front();
            if (files.size() == 2)
                chosen = assignImportFiles(files.front(), files.back());
            onChosen(chosen);
        });
}

std::optional<std::vector<TakeImport::Signal>> TakeImport::read(const ImportFiles& files)
{
    std::vector<Signal> signals;
    for (const bool processed : {false, true})
    {
        const auto& file = processed ? files.processed : files.di;
        if (file.empty())
            continue;
        auto content = reader_.read(file);
        if (!content)
        {
            showError(i18n::tr("\"%1\" could not be read as an audio file.", fileName(file)));
            return std::nullopt;
        }
        signals.push_back({std::move(*content), processed});
    }
    return signals;
}

void TakeImport::importSignals(Request request)
{
    // The folder may be reported later, after a new project was saved; the samples are shared, not copied.
    auto shared = std::make_shared<const Request>(std::move(request));
    environment_.projectFolder(
        [this, shared](const std::optional<std::filesystem::path>& folder)
        {
            if (folder)
                write(*folder, *shared);
        });
}

void TakeImport::write(const std::filesystem::path& folder, const Request& request)
{
    model::Project project{project_, &undoManager_};
    std::optional<model::Voice> voice;
    if (request.voiceId)
    {
        voice = project.findVoice(*request.voiceId);
        if (!voice)
            return; // removed while the dialog was open
    }
    if (!fileSystem_.createDirectories(folder / "audio"))
    {
        showError(i18n::tr("The folder for the recordings could not be created in \"%1\".",
                           io::utf8FromPath(folder)));
        return;
    }

    const bool firstTake = !hasTakes();
    const double sampleRate = firstTake ? request.signals.front().content.sampleRate : project.sampleRate();
    model::TakeData take;
    take.id = idGenerator_.next();
    take.recordedAt = environment_.now();
    if (!writeFiles(folder, baseName(voice, request.newVoiceName, folder), sampleRate, request.signals, take))
        return;
    if (firstTake)
        project_.setProperty(model::ids::sampleRate, sampleRate, nullptr);
    addToProject(project, voice, request.newVoiceName, take);
}

std::string TakeImport::baseName(const std::optional<model::Voice>& voice,
                                 const std::string& newVoiceName,
                                 const std::filesystem::path& folder) const
{
    if (voice)
        return nextTakeBaseName(voice->name(), voice->numTakes(), folder, fileSystem_);
    return nextTakeBaseName(newVoiceName, 0, folder, fileSystem_);
}

void TakeImport::addToProject(model::Project& project,
                              std::optional<model::Voice> voice,
                              const std::string& newVoiceName,
                              const model::TakeData& take)
{
    if (voice)
        undoManager_.beginNewTransaction();
    else
    {
        // One undo step with the take: addVoice starts it.
        voice = addVoice(project, idGenerator_, environment_.numInputs());
        voice->setName(newVoiceName);
    }
    std::vector<std::string> added;
    added.push_back(voice->addTake(take).id());
    environment_.takesImported(added);
}

bool TakeImport::writeFiles(const std::filesystem::path& folder,
                            const std::string& base,
                            double sampleRate,
                            const std::vector<Signal>& signals,
                            model::TakeData& take)
{
    for (const auto& signal : signals)
    {
        const auto relative = takeAudioFile(base, signal.processed);
        auto file = fileSystem_.createBinaryFile(folder / io::pathFromUtf8(relative));
        if (!file)
        {
            showError(i18n::tr("\"%1\" could not be created.", relative));
            return false;
        }
        io::WavWriter writer{std::move(file), sampleRate};
        // A failed append makes updateHeader() fail as well.
        static_cast<void>(writer.append(monoAtRate(signal.content, sampleRate)));
        if (!writer.updateHeader())
        {
            showError(i18n::tr("Writing \"%1\" failed. Is the disk full?", relative));
            return false;
        }
        (signal.processed ? take.wetFile : take.diFile) = relative;
    }
    return true;
}

bool TakeImport::hasTakes() const
{
    const model::Project project{project_, nullptr};
    for (int index = 0; index < project.numVoices(); ++index)
        if (project.voice(index).numTakes() > 0)
            return true;
    return false;
}

void TakeImport::showError(const std::string& message) const
{
    environment_.showMessage(i18n::tr("Import"), message);
}

} // namespace snotes::ui
