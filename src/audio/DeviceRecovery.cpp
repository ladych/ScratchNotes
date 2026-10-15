#include "audio/DeviceRecovery.h"

namespace snotes::audio
{

RecoveryAction DeviceRecovery::tick(const DeviceObservation& observation)
{
    const bool running = observation.open && observation.audioBlocks != lastBlocks_;
    lastBlocks_ = observation.audioBlocks;
    silentTicks_ = running ? 0 : silentTicks_ + 1;

    if (observation.onPreferred)
        triedList_.reset();

    if (stalled() && (silentTicks_ - kStallTicks) % kRetryTicks == 0)
    {
        triedList_ = observation.deviceList;
        return RecoveryAction::reopen;
    }
    if (!observation.hasPreferred || observation.onPreferred || stalled())
        return RecoveryAction::none;
    if (!triedList_)
    {
        // The fallback just took over after a failed attempt: wait for the next change of the list.
        triedList_ = observation.deviceList;
        return RecoveryAction::none;
    }
    if (*triedList_ == observation.deviceList)
        return RecoveryAction::none;
    triedList_ = observation.deviceList;
    return RecoveryAction::reopen;
}

} // namespace snotes::audio
