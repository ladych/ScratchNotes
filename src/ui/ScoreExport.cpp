#include "ui/ScoreExport.h"

#include "i18n/Translate.h"
#include "io/Utf8Path.h"
#include "model/Project.h"
#include "ui/RhythmUpdate.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

namespace snotes::ui
{
namespace
{

constexpr std::string_view kRefusedCharacters = "<>:\"/\\|?*";

/// A file name without characters that file systems refuse; "score" for none.
std::string fileNameOf(std::string name)
{
    std::ranges::replace_if(
        name,
        [](char character) { return kRefusedCharacters.find(character) != std::string_view::npos; },
        '_');
    if (name.empty())
        return "score";
    return name;
}

const char* sourceOf(ExportFormat format)
{
    if (format == ExportFormat::midi)
        return kMidiFile;
    return kScoreFile;
}

const char* extensionOf(ExportFormat format)
{
    if (format == ExportFormat::midi)
        return ".mid";
    return ".musicxml";
}

} // namespace

ScoreExport::ScoreExport(juce::ValueTree project,
                         io::IFileSystem& fileSystem,
                         IDocumentView& view,
                         FolderProvider folder)
    : project_(std::move(project)), fileSystem_(fileSystem), view_(view), folder_(std::move(folder))
{
}

void ScoreExport::exportAs(ExportFormat format)
{
    const model::Project project{project_, nullptr};
    const auto folder = folder_();
    const bool detected = !project.beatGrid().scoreFile().empty();
    std::filesystem::path source;
    if (folder && detected)
        source = *folder / sourceOf(format);
    if (source.empty() || !fileSystem_.exists(source))
    {
        view_.showMessage(i18n::tr("Export"), i18n::tr("There is no score yet. Detect the notes first."));
        return;
    }
    // The dialog may answer later; the source and the extension wait for it.
    source_ = source;
    extension_ = extensionOf(format);
    const auto suggested = source.parent_path() / io::pathFromUtf8(fileNameOf(project.name()) + extension_);
    view_.chooseExportFile(suggested,
                           "*" + extension_,
                           [this](const std::optional<std::filesystem::path>& chosen)
                           {
                               if (chosen)
                                   copyTo(*chosen);
                           });
}

void ScoreExport::copyTo(std::filesystem::path target)
{
    if (target.extension() != extension_)
        target += extension_;
    if (!fileSystem_.copyFile(source_, target))
        view_.showMessage(i18n::tr("Export"),
                          i18n::tr("The score could not be written to %1.", io::utf8FromPath(target)));
}

} // namespace snotes::ui
