#pragma once

#include "ui/TakeAudioCache.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace snotes::app
{

/// Humble object (E-03): reads the first channel of a take file with JUCE's WAV reader (F-WI-02).
class JuceTakeAudioLoader final : public ui::ITakeAudioLoader
{
public:
    JuceTakeAudioLoader() { formats_.registerBasicFormats(); }

    [[nodiscard]] std::shared_ptr<const engine::TakeAudio> load(const std::filesystem::path& file) override;

private:
    juce::AudioFormatManager formats_;
};

} // namespace snotes::app
