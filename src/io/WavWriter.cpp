#include "io/WavWriter.h"

#include <array>
#include <bit>
#include <string_view>
#include <utility>
#include <vector>

namespace snotes::io
{
namespace
{

static_assert(std::endian::native == std::endian::little, "WAV files are little-endian");

constexpr std::uint16_t kFormatIeeeFloat = 3;
constexpr std::uint16_t kBitsPerSample = 32;
constexpr std::uint32_t kBytesPerSample = kBitsPerSample / 8;
constexpr std::uint32_t kFormatChunkSize = 18;
constexpr std::uint32_t kFactChunkSize = 4;
// Positions of the sizes that grow while recording.
constexpr std::uint64_t kRiffSizeOffset = 4;
constexpr std::uint64_t kFactLengthOffset = 46;
constexpr std::uint64_t kDataSizeOffset = 54;

class ByteBuilder
{
public:
    void text(std::string_view chunkId)
    {
        for (const char c : chunkId)
            bytes_.push_back(static_cast<std::byte>(c));
    }

    template <typename Integer> void number(Integer value)
    {
        const auto raw = std::bit_cast<std::array<std::byte, sizeof(Integer)>>(value);
        bytes_.insert(bytes_.end(), raw.begin(), raw.end());
    }

    [[nodiscard]] std::span<const std::byte> bytes() const { return bytes_; }

private:
    std::vector<std::byte> bytes_;
};

std::array<std::byte, 4> littleEndian(std::uint32_t value)
{
    return std::bit_cast<std::array<std::byte, 4>>(value);
}

} // namespace

WavWriter::WavWriter(std::unique_ptr<IBinaryFile> file, double sampleRate) : file_(std::move(file))
{
    const auto rate = static_cast<std::uint32_t>(sampleRate);
    ByteBuilder header;
    header.text("RIFF");
    header.number<std::uint32_t>(kHeaderSize - 8);
    header.text("WAVE");
    header.text("fmt ");
    header.number(kFormatChunkSize);
    header.number(kFormatIeeeFloat);
    header.number<std::uint16_t>(1); // mono
    header.number(rate);
    header.number(rate * kBytesPerSample);
    header.number(static_cast<std::uint16_t>(kBytesPerSample));
    header.number(kBitsPerSample);
    header.number<std::uint16_t>(0); // no extension
    header.text("fact");
    header.number(kFactChunkSize);
    header.number<std::uint32_t>(0);
    header.text("data");
    header.number<std::uint32_t>(0);
    check(file_->append(header.bytes()));
}

bool WavWriter::append(std::span<const float> samples)
{
    if (!ok_)
        return false;
    numSamples_ += samples.size();
    return check(file_->append(std::as_bytes(samples)));
}

bool WavWriter::updateHeader()
{
    if (!ok_)
        return false;
    const auto dataBytes = static_cast<std::uint32_t>(numSamples_ * kBytesPerSample);
    const auto riffSize = littleEndian(kHeaderSize - 8 + dataBytes);
    const auto length = littleEndian(static_cast<std::uint32_t>(numSamples_));
    const auto dataSize = littleEndian(dataBytes);
    return check(file_->overwrite(kRiffSizeOffset, riffSize) && file_->overwrite(kFactLengthOffset, length) &&
                 file_->overwrite(kDataSizeOffset, dataSize) && file_->flush());
}

bool WavWriter::check(bool written)
{
    // Only called while ok_ is still true.
    ok_ = written;
    return ok_;
}

} // namespace snotes::io
