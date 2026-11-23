#include "ui/TakeFileNames.h"

#include "i18n/Translate.h"
#include "io/Utf8Path.h"

#include <juce_core/juce_core.h>

namespace snotes::ui
{

std::string takeAudioFile(const std::string& base, bool processed)
{
    std::string suffix = " DI";
    if (processed)
        suffix = i18n::tr(" processed");
    return "audio/" + base + suffix + ".wav";
}

std::string nextTakeBaseName(const std::string& voiceName,
                             int numTakes,
                             const std::filesystem::path& folder,
                             const io::IFileSystem& fileSystem)
{
    const auto name =
        juce::File::createLegalFileName(juce::String::fromUTF8(voiceName.c_str())).toStdString();
    for (int number = numTakes + 1;; ++number)
    {
        auto base = i18n::tr("%1 take %2", name, std::to_string(number));
        const auto di = folder / io::pathFromUtf8(takeAudioFile(base, false));
        const auto processed = folder / io::pathFromUtf8(takeAudioFile(base, true));
        if (!fileSystem.exists(di) && !fileSystem.exists(processed))
            return base;
    }
}

} // namespace snotes::ui
