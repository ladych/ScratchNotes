#include "rhythm/Fft.h"

#include <cmath>
#include <numbers>

namespace snotes::rhythm
{

PowerSpectrum::PowerSpectrum(std::size_t size)
    : size_(size), reversed_(size), twiddles_(size / 2), buffer_(size)
{
    std::size_t bits = 0;
    while ((std::size_t{1} << bits) < size)
        ++bits;
    for (std::size_t index = 0; index < size; ++index)
    {
        std::size_t reversed = 0;
        for (std::size_t bit = 0; bit < bits; ++bit)
            reversed |= ((index >> bit) & 1U) << (bits - 1 - bit);
        reversed_[index] = reversed;
    }
    for (std::size_t index = 0; index < size / 2; ++index)
        twiddles_[index] =
            std::polar(1.0, -2.0 * std::numbers::pi * static_cast<double>(index) / static_cast<double>(size));
}

void PowerSpectrum::compute(std::span<const double> frame, std::span<double> power)
{
    for (std::size_t index = 0; index < size_; ++index)
        buffer_[reversed_[index]] = frame[index];
    for (std::size_t length = 2; length <= size_; length *= 2)
    {
        const std::size_t half = length / 2;
        const std::size_t stride = size_ / length;
        for (std::size_t start = 0; start < size_; start += length)
            for (std::size_t offset = 0; offset < half; ++offset)
            {
                const auto odd = buffer_[start + offset + half] * twiddles_[offset * stride];
                buffer_[start + offset + half] = buffer_[start + offset] - odd;
                buffer_[start + offset] += odd;
            }
    }
    for (std::size_t bin = 0; bin <= size_ / 2; ++bin)
        power[bin] = std::norm(buffer_[bin]);
}

} // namespace snotes::rhythm
