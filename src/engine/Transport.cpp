#include "engine/Transport.h"

#include <algorithm>

namespace snotes::engine
{

void Transport::locate(std::int64_t position) noexcept
{
    locate_.store(std::max<std::int64_t>(position, 0));
}

TransportBlock Transport::beginBlock() noexcept
{
    const std::int64_t target = locate_.exchange(kNoLocate);
    if (target != kNoLocate)
        position_.store(target);
    TransportBlock block;
    block.playing = playing_.load();
    block.position = position_.load();
    return block;
}

void Transport::endBlock(const TransportBlock& block, int numSamples) noexcept
{
    if (block.playing)
        position_.store(block.position + numSamples);
}

} // namespace snotes::engine
