#include "io/TakeFileWriter.h"

#include <algorithm>
#include <span>
#include <utility>

namespace snotes::io
{
namespace
{

constexpr std::size_t kChunkSamples = 8192;

} // namespace

TakeFileWriter::TakeFileWriter(engine::RecordingSession& session,
                               std::vector<std::unique_ptr<WavWriter>> writers,
                               std::uint64_t headerInterval)
    : session_(session), writers_(std::move(writers)), headerInterval_(headerInterval), buffer_(kChunkSamples)
{
}

void TakeFileWriter::pump()
{
    std::uint64_t written = 0;
    for (std::size_t channel = 0; channel < writers_.size(); ++channel)
    {
        auto& ring = session_.ring(channel);
        std::uint64_t channelWritten = 0;
        while (const auto count = ring.read(buffer_.data(), buffer_.size()))
        {
            if (!writers_[channel]->append(std::span{buffer_}.first(count)))
                failed_.store(true);
            channelWritten += count;
        }
        written = std::max(written, channelWritten);
    }
    samplesSinceHeader_ += written;
    if (samplesSinceHeader_ >= headerInterval_)
        updateHeaders();
}

bool TakeFileWriter::finish()
{
    pump();
    updateHeaders();
    return !failed();
}

void TakeFileWriter::updateHeaders()
{
    samplesSinceHeader_ = 0;
    for (auto& writer : writers_)
        if (!writer->updateHeader())
            failed_.store(true);
}

} // namespace snotes::io
