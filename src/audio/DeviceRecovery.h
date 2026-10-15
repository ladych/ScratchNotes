#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace snotes::audio
{

/// What the GUI sees of the audio device once per second.
struct DeviceObservation
{
    bool hasPreferred = false;     ///< the user chose a device and it is saved (F-AO-04)
    bool open = false;             ///< a device is open
    bool onPreferred = false;      ///< the open device is the chosen one, not a fallback
    std::uint64_t audioBlocks = 0; ///< driver callbacks so far
    std::size_t deviceList = 0;    ///< fingerprint of the device names of all driver types
};

enum class RecoveryAction : std::uint8_t
{
    none,
    /// Open the chosen device again, or a fallback if that fails.
    reopen
};

/// Keeps the audio running when the interface is unplugged or the driver hangs (Q-09). A device that
/// is closed or gets no callbacks for three seconds (e.g. ASIO after unplugging, where the driver stays
/// listed) is reopened, and again every five seconds. While a fallback device plays, the chosen device
/// is tried again whenever the device list changes, e.g. when the interface is plugged back in.
/// GUI thread only.
class DeviceRecovery
{
public:
    /// Called once per second.
    [[nodiscard]] RecoveryAction tick(const DeviceObservation& observation);
    /// No audio for three seconds or more.
    [[nodiscard]] bool stalled() const { return silentTicks_ >= kStallTicks; }

private:
    static constexpr int kStallTicks = 3;
    static constexpr int kRetryTicks = 5;

    int silentTicks_ = 0;
    std::uint64_t lastBlocks_ = 0;
    /// Device list at the last attempt to open the chosen device, while a fallback plays.
    std::optional<std::size_t> triedList_;
};

} // namespace snotes::audio
