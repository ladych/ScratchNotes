#include "io/AudioFileReader.h"

#include "io/Utf8Path.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <limits>
#include <memory>

namespace snotes::io
{

std::optional<AudioFileContent> AudioFileReader::read(const std::filesystem::path& file)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    const juce::File source{juce::String::fromUTF8(utf8FromPath(file).c_str())};
    const std::unique_ptr<juce::AudioFormatReader> reader{formats.createReaderFor(source)};
    if (reader == nullptr)
        return std::nullopt;
    // JUCE's buffers hold at most 2^31 - 1 samples, more than three hours at 192 kHz; the rest is left out.
    const auto length =
        static_cast<int>(std::min<juce::int64>(reader->lengthInSamples, std::numeric_limits<int>::max()));
    if (length <= 0)
        return std::nullopt;
    const auto numChannels = static_cast<int>(reader->numChannels);
    juce::AudioBuffer<float> buffer{numChannels, length};
    // A file cut short (e.g. by a crash while it was written) reads as silence after its end.
    static_cast<void>(reader->read(&buffer, 0, length, 0, true, true));
    AudioFileContent content;
    content.sampleRate = reader->sampleRate;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float* const samples = buffer.getReadPointer(channel);
        content.channels.emplace_back(samples, samples + length);
    }
    return content;
}

} // namespace snotes::io
