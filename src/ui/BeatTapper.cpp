#include "ui/BeatTapper.h"

#include "i18n/Translate.h"

#include <utility>

namespace snotes::ui
{

BeatTapper::BeatTapper(GridCorrection& correction, TapEnvironment environment)
    : correction_(correction), environment_(std::move(environment))
{
}

void BeatTapper::tap()
{
    if (environment_.isPlaying())
        taps_.push_back(environment_.positionSeconds() - environment_.outputLatencySeconds());
}

void BeatTapper::poll()
{
    if (taps_.empty() || environment_.isPlaying())
        return;
    correction_.replaceBeats(std::exchange(taps_, {}));
}

std::string BeatTapper::statusText() const
{
    if (taps_.empty())
        return {};
    return i18n::tr("Tapping the beat: %1 beats, they replace the grid when the playback stops",
                    std::to_string(taps_.size()));
}

} // namespace snotes::ui
