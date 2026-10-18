#include "rhythm/OnsetStrength.h"

#include "rhythm/Fft.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace snotes::rhythm
{
namespace
{

constexpr std::size_t kFftSize = 2048;
constexpr std::size_t kBins = (kFftSize / 2) + 1;
constexpr std::size_t kMelBands = 128;
constexpr double kAmin = 1e-10;
constexpr double kTopDb = 80.0;

// Slaney's mel scale (librosa.hz_to_mel with htk=False): linear below 1 kHz, logarithmic above.
constexpr double kLinearHzPerMel = 200.0 / 3.0;
constexpr double kLogStartHz = 1000.0;
constexpr double kLogStartMel = kLogStartHz / kLinearHzPerMel;
const double kLogStep = std::log(6.4) / 27.0;

double melToHz(double mel)
{
    if (mel >= kLogStartMel)
        return kLogStartHz * std::exp(kLogStep * (mel - kLogStartMel));
    return mel * kLinearHzPerMel;
}

/// A triangular filter of librosa.filters.mel(norm="slaney"): weights of the bins first, first + 1, ...
struct MelBand
{
    std::size_t first = 0;
    std::vector<double> weights;
};

std::vector<MelBand> melBands()
{
    std::vector<double> edges(kMelBands + 2);
    // The Nyquist frequency on the logarithmic part of the scale.
    const double top = kLogStartMel + (std::log(kAnalysisRate / 2.0 / kLogStartHz) / kLogStep);
    for (std::size_t index = 0; index < edges.size(); ++index)
        edges[index] = melToHz(top * static_cast<double>(index) / static_cast<double>(kMelBands + 1));
    std::vector<MelBand> bands(kMelBands);
    for (std::size_t band = 0; band < kMelBands; ++band)
    {
        const double norm = 2.0 / (edges[band + 2] - edges[band]);
        bands[band].first = kBins;
        for (std::size_t bin = 0; bin < kBins; ++bin)
        {
            const double hz = static_cast<double>(bin) * kAnalysisRate / static_cast<double>(kFftSize);
            const double lower = (hz - edges[band]) / (edges[band + 1] - edges[band]);
            const double upper = (edges[band + 2] - hz) / (edges[band + 2] - edges[band + 1]);
            const double weight = std::max(0.0, std::min(lower, upper));
            if (weight > 0.0)
            {
                bands[band].first = std::min(bands[band].first, bin);
                bands[band].weights.resize(bin + 1 - bands[band].first);
                bands[band].weights[bin - bands[band].first] = weight * norm;
            }
        }
    }
    return bands;
}

std::vector<double> hannWindow()
{
    std::vector<double> window(kFftSize);
    for (std::size_t index = 0; index < kFftSize; ++index)
        window[index] = 0.5 - (0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(index) /
                                              static_cast<double>(kFftSize)));
    return window;
}

/// The power of the bands in dB.
void bandDecibels(const std::vector<MelBand>& bands,
                  const std::vector<double>& power,
                  std::vector<float>& decibels)
{
    for (std::size_t band = 0; band < kMelBands; ++band)
    {
        double energy = 0.0;
        for (std::size_t bin = 0; bin < bands[band].weights.size(); ++bin)
            energy += bands[band].weights[bin] * power[bands[band].first + bin];
        decibels[band] = static_cast<float>(10.0 * std::log10(std::max(kAmin, energy)));
    }
}

/// Mel power spectrogram in dB, frame by frame.
std::vector<std::vector<float>> melDecibels(std::span<const float> audio, int hop)
{
    const auto size = static_cast<std::ptrdiff_t>(audio.size());
    const auto bands = melBands();
    const auto window = hannWindow();
    PowerSpectrum spectrum{kFftSize};
    std::vector<double> frame(kFftSize);
    std::vector<double> power(kBins);
    std::vector<std::vector<float>> result(1 + (audio.size() / static_cast<std::size_t>(hop)),
                                           std::vector<float>(kMelBands));
    for (std::size_t index = 0; index < result.size(); ++index)
    {
        const auto first =
            (static_cast<std::ptrdiff_t>(index) * hop) - static_cast<std::ptrdiff_t>(kFftSize / 2);
        for (std::size_t offset = 0; offset < kFftSize; ++offset)
        {
            const auto sample = first + static_cast<std::ptrdiff_t>(offset);
            frame[offset] =
                sample >= 0 && sample < size ? window[offset] * audio[static_cast<std::size_t>(sample)] : 0.0;
        }
        spectrum.compute(frame, power);
        bandDecibels(bands, power, result[index]);
    }
    return result;
}

} // namespace

std::vector<float> onsetStrength(std::span<const float> audio, int hop)
{
    auto decibels = melDecibels(audio, hop);
    float loudest = -std::numeric_limits<float>::infinity();
    for (const auto& frame : decibels)
        loudest = std::max(loudest, *std::ranges::max_element(frame));
    for (auto& frame : decibels)
        for (auto& value : frame)
            value = std::max(value, loudest - static_cast<float>(kTopDb));
    // librosa pads the differences at the front by the lag and half a frame, so they line up with the frames.
    const std::size_t padding = 1 + (kFftSize / (2 * static_cast<std::size_t>(hop)));
    std::vector<float> envelope(decibels.size());
    for (std::size_t frame = 1; frame < decibels.size() && frame - 1 + padding < envelope.size(); ++frame)
    {
        double rise = 0.0;
        for (std::size_t band = 0; band < kMelBands; ++band)
            rise += std::max(0.0F, decibels[frame][band] - decibels[frame - 1][band]);
        envelope[frame - 1 + padding] = static_cast<float>(rise / kMelBands);
    }
    return envelope;
}

double frameSeconds(std::size_t frame, int hop)
{
    return static_cast<double>(frame) * hop / kAnalysisRate;
}

} // namespace snotes::rhythm
