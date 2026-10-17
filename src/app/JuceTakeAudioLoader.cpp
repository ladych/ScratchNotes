#include "app/JuceTakeAudioLoader.h"

#include "io/Utf8Path.h"

#include <limits>
#include <memory>
#include <vector>

namespace snotes::app
{

std::shared_ptr<const engine::TakeAudio> JuceTakeAudioLoader::load(const std::filesystem::path& file)
{
    const juce::File source{juce::String::fromUTF8(io::utf8FromPath(file).c_str())};
    const std::unique_ptr<juce::AudioFormatReader> reader{formats_.createReaderFor(source)};
    if (reader == nullptr || reader->lengthInSamples > std::numeric_limits<int>::max())
        return nullptr;
    const auto length = static_cast<int>(reader->lengthInSamples);
    juce::AudioBuffer<float> buffer{1, length};
    if (!reader->read(&buffer, 0, length, 0, true, false))
        return nullptr;
    const float* const samples = buffer.getReadPointer(0);
    return std::make_shared<const engine::TakeAudio>(std::vector<float>(samples, samples + length));
}

} // namespace snotes::app
