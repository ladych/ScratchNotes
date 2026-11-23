#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace snotes::io
{

/// The samples of an audio file, one vector per channel.
struct AudioFileContent
{
    double sampleRate = 0.0;
    std::vector<std::vector<float>> channels;
};

/// Reads whole audio files, so the import can be tested without decoding real files (E-06).
class IAudioFileReader
{
public:
    virtual ~IAudioFileReader() = default;

    /// std::nullopt if the file is missing, not an audio file or has no samples.
    [[nodiscard]] virtual std::optional<AudioFileContent> read(const std::filesystem::path& file) = 0;

protected:
    IAudioFileReader() = default;
    IAudioFileReader(const IAudioFileReader&) = default;
    IAudioFileReader(IAudioFileReader&&) = default;
    IAudioFileReader& operator=(const IAudioFileReader&) = default;
    IAudioFileReader& operator=(IAudioFileReader&&) = default;
};

/// Reads WAV, AIFF, FLAC and Ogg Vorbis files with JUCE's readers (F-PJ-05), in any bit depth.
class AudioFileReader final : public IAudioFileReader
{
public:
    [[nodiscard]] std::optional<AudioFileContent> read(const std::filesystem::path& file) override;
};

} // namespace snotes::io
