#include "ui/TransportControl.h"

#include <cmath>
#include <utility>

namespace snotes::ui
{

TransportControl::TransportControl(engine::Transport& transport, TransportEnvironment environment)
    : transport_(transport), environment_(std::move(environment))
{
}

void TransportControl::togglePlay()
{
    if (environment_.isRecording())
        environment_.stopRecording();
    else if (transport_.isPlaying())
        transport_.stop();
    else
        transport_.play();
}

void TransportControl::returnToStart()
{
    locate(0.0);
}

void TransportControl::locate(double seconds)
{
    const double rate = environment_.sampleRate();
    if (!environment_.isRecording() && rate > 0.0)
        transport_.locate(std::llround(seconds * rate));
}

double TransportControl::positionSeconds() const
{
    const double rate = environment_.sampleRate();
    if (rate <= 0.0)
        return 0.0;
    return static_cast<double>(transport_.position()) / rate;
}

} // namespace snotes::ui
