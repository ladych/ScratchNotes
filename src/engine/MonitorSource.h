#pragma once

#include <cstdint>

namespace snotes::engine
{

/// Which input of a voice is heard while playing (software monitoring, F-WI-06).
enum class MonitorSource : std::uint8_t
{
    off,
    processed,
    di
};

} // namespace snotes::engine
