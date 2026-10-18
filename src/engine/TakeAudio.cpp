#include "engine/TakeAudio.h"

#include <algorithm>

namespace snotes::engine
{

void TakeAudio::read(std::int64_t start, int numSamples, float* destination) const noexcept
{
    std::fill_n(destination, numSamples, 0.0F);
    const std::int64_t first = std::max<std::int64_t>(start, 0);
    const std::int64_t last = std::min<std::int64_t>(start + numSamples, length());
    if (first >= last)
        return;
    std::copy(samples_.begin() + first, samples_.begin() + last, destination + (first - start));
}

} // namespace snotes::engine
