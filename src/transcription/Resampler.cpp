#include "transcription/Resampler.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace snotes::transcription
{
namespace
{

/// Zero crossings of the sinc on each side: the longer, the steeper the low-pass.
constexpr int kZeroCrossings = 32;
/// Kaiser window for about 90 dB stop-band attenuation.
constexpr double kKaiserBeta = 8.6;
/// The pass band ends a little below the new Nyquist frequency.
constexpr double kRolloff = 0.95;
constexpr int kTableStepsPerCrossing = 512;
constexpr int kTableSize = (kZeroCrossings * kTableStepsPerCrossing) + 2;

/// sinc(u * kZeroCrossings) * kaiser(u) for u from 0 to 1, read with linear interpolation.
class KernelTable
{
public:
    KernelTable() : values_(kTableSize)
    {
        const double norm = std::cyl_bessel_i(0.0, kKaiserBeta);
        for (int step = 0; step < kTableSize; ++step)
        {
            const double u = std::min(1.0, static_cast<double>(step) / (kTableSize - 2));
            const double x = std::numbers::pi * u * kZeroCrossings;
            const double sinc = step == 0 ? 1.0 : std::sin(x) / x;
            const double window = std::cyl_bessel_i(0.0, kKaiserBeta * std::sqrt(1.0 - (u * u))) / norm;
            values_[static_cast<std::size_t>(step)] = sinc * window;
        }
    }

    /// u = |distance| / half width, 0..1.
    [[nodiscard]] double at(double u) const
    {
        const double position = u * (kTableSize - 2);
        const auto index = static_cast<std::size_t>(position);
        const double fraction = position - static_cast<double>(index);
        return values_[index] + (fraction * (values_[index + 1] - values_[index]));
    }

private:
    std::vector<double> values_;
};

} // namespace

std::vector<float> resample(std::span<const float> input, double fromRate, double toRate)
{
    if (fromRate == toRate)
        return {input.begin(), input.end()};
    const double ratio = toRate / fromRate;
    const double cutoff = kRolloff * std::min(1.0, ratio);
    const double halfWidth = kZeroCrossings / cutoff;
    const auto inputSize = static_cast<std::ptrdiff_t>(input.size());
    const KernelTable kernel;
    std::vector<float> output(static_cast<std::size_t>(std::ceil(static_cast<double>(input.size()) * ratio)));
    for (std::size_t index = 0; index < output.size(); ++index)
    {
        const double centre = static_cast<double>(index) / ratio;
        const auto first =
            std::max<std::ptrdiff_t>(0, static_cast<std::ptrdiff_t>(std::ceil(centre - halfWidth)));
        const auto last = std::min<std::ptrdiff_t>(
            inputSize - 1, static_cast<std::ptrdiff_t>(std::floor(centre + halfWidth)));
        double sum = 0.0;
        for (auto sample = first; sample <= last; ++sample)
            sum += input[static_cast<std::size_t>(sample)] *
                   kernel.at(std::abs(centre - static_cast<double>(sample)) / halfWidth);
        output[index] = static_cast<float>(cutoff * sum);
    }
    return output;
}

} // namespace snotes::transcription
