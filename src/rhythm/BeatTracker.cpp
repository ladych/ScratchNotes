#include "rhythm/BeatTracker.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>

namespace snotes::rhythm
{
namespace
{

constexpr double kSlowestBpm = 40.0;
constexpr double kFastestBpm = 220.0;
constexpr double kPriorCentreBpm = 100.0;
constexpr double kSameTempo = 0.04;
constexpr std::size_t kMaximumCandidates = 4;
constexpr double kTiny = 1e-9;

/// Autocorrelation of the mean-free envelope for the lags 0 .. lags - 1, normalised to lag 0.
std::vector<double> autocorrelation(std::span<const float> envelope, std::size_t lags)
{
    const double mean =
        std::accumulate(envelope.begin(), envelope.end(), 0.0) / static_cast<double>(envelope.size());
    std::vector<double> values(lags);
    for (std::size_t lag = 0; lag < lags && lag < envelope.size(); ++lag)
        for (std::size_t index = lag; index < envelope.size(); ++index)
            values[lag] += (envelope[index] - mean) * (envelope[index - lag] - mean);
    const double zero = values[0] + kTiny;
    for (auto& value : values)
        value /= zero;
    return values;
}

} // namespace

namespace
{

/// Autocorrelation weighted by the tempo prior, per lag - 1; zero outside 40 to 220 BPM.
std::vector<double> tempoScores(std::span<const float> envelope, double framesPerSecond)
{
    // Lags beyond the slowest tempo score zero; one more lag decides whether the last one is a peak.
    const auto lags = static_cast<std::size_t>(60.0 * framesPerSecond / kSlowestBpm) + 3;
    const auto correlation = autocorrelation(envelope, lags);
    std::vector<double> score(lags - 1);
    for (std::size_t index = 0; index < score.size(); ++index)
    {
        const double bpm = 60.0 * framesPerSecond / static_cast<double>(index + 1);
        const double prior = std::exp(-0.5 * std::pow(std::log2(bpm / kPriorCentreBpm), 2.0));
        if (bpm >= kSlowestBpm && bpm <= kFastestBpm)
            score[index] = std::max(correlation[index + 1], 0.0) * prior;
    }
    return score;
}

/// The local maxima of the scores, best first.
std::vector<std::size_t> peaksByScore(const std::vector<double>& score)
{
    std::vector<std::size_t> peaks;
    for (std::size_t index = 1; index + 1 < score.size(); ++index)
        if (score[index] > 0.0 && score[index] >= score[index - 1] && score[index] >= score[index + 1])
            peaks.push_back(index);
    std::ranges::stable_sort(peaks, [&score](std::size_t a, std::size_t b) { return score[a] > score[b]; });
    return peaks;
}

} // namespace

std::vector<TempoCandidate> tempoCandidates(std::span<const float> envelope, double framesPerSecond)
{
    if (envelope.empty())
        return {};
    const auto score = tempoScores(envelope, framesPerSecond);
    std::vector<TempoCandidate> candidates;
    double total = 0.0;
    for (const auto peak : peaksByScore(score))
    {
        const double bpm = 60.0 * framesPerSecond / static_cast<double>(peak + 1);
        if (std::ranges::any_of(candidates,
                                [bpm](const TempoCandidate& candidate)
                                { return std::abs(bpm - candidate.bpm) / candidate.bpm < kSameTempo; }))
            continue;
        candidates.push_back({.bpm = bpm, .share = score[peak]});
        total += score[peak];
        if (candidates.size() == kMaximumCandidates)
            break;
    }
    for (auto& candidate : candidates)
        candidate.share /= total;
    return candidates;
}

namespace
{

/// The envelope divided by its standard deviation and smoothed with a Gaussian window of +-1 beat, as
/// librosa's __beat_local_score (which leaves out the last tap of the window).
std::vector<double> localScore(std::span<const float> envelope, std::ptrdiff_t framesPerBeat)
{
    const auto size = static_cast<std::ptrdiff_t>(envelope.size());
    const double mean = std::accumulate(envelope.begin(), envelope.end(), 0.0) / static_cast<double>(size);
    double squares = 0.0;
    for (const float value : envelope)
        squares += (value - mean) * (value - mean);
    const double deviation =
        std::sqrt(squares / static_cast<double>(size - 1)) + std::numeric_limits<float>::min();
    const std::ptrdiff_t taps = (2 * framesPerBeat) + 1;
    std::vector<double> window(static_cast<std::size_t>(taps));
    for (std::ptrdiff_t tap = 0; tap < taps; ++tap)
        window[static_cast<std::size_t>(tap)] =
            std::exp(-0.5 * std::pow(static_cast<double>(tap - framesPerBeat) * 32.0 /
                                         static_cast<double>(framesPerBeat),
                                     2.0));
    std::vector<double> score(envelope.size());
    for (std::ptrdiff_t index = 0; index < size; ++index)
        for (std::ptrdiff_t tap = std::max<std::ptrdiff_t>(0, index + framesPerBeat - size + 1);
             tap < std::min(index + framesPerBeat, taps);
             ++tap)
            score[static_cast<std::size_t>(index)] +=
                window[static_cast<std::size_t>(tap)] *
                envelope[static_cast<std::size_t>(index + framesPerBeat - tap)] / deviation;
    return score;
}

struct Path
{
    std::vector<std::ptrdiff_t> backlink;
    std::vector<double> cumulative;
};

/// The frames before a beat where the previous beat may be, and how strongly the tempo holds.
struct SearchWindow
{
    std::ptrdiff_t nearest = 0;
    std::ptrdiff_t farthest = 0;
    double logBeat = 0.0;
    double tightness = 0.0;
};

struct Predecessor
{
    double score = -std::numeric_limits<double>::infinity();
    std::ptrdiff_t location = -1;
};

/// The previous beat with the best cumulative score minus the penalty for leaving the tempo.
Predecessor
bestPredecessor(const std::vector<double>& cumulative, std::ptrdiff_t index, const SearchWindow& window)
{
    Predecessor best;
    for (std::ptrdiff_t previous = index - window.nearest;
         previous >= 0 && previous >= index - window.farthest;
         --previous)
    {
        const double candidate =
            cumulative[static_cast<std::size_t>(previous)] -
            (window.tightness *
             std::pow(std::log(static_cast<double>(index - previous)) - window.logBeat, 2.0));
        if (candidate > best.score)
            best = {.score = candidate, .location = previous};
    }
    return best;
}

/// librosa's __beat_track_dp: the best predecessor of every frame within the search window.
Path dynamicProgram(const std::vector<double>& score,
                    std::ptrdiff_t framesPerBeat,
                    const BeatTrackerSettings& settings)
{
    const double threshold = 0.01 * *std::ranges::max_element(score);
    const auto beat = static_cast<double>(framesPerBeat);
    const SearchWindow window{
        .nearest = static_cast<std::ptrdiff_t>(std::nearbyint(beat * settings.shortestBeat)),
        .farthest = static_cast<std::ptrdiff_t>(std::nearbyint(beat * settings.longestBeat)),
        .logBeat = std::log(beat),
        .tightness = settings.tightness};
    Path path;
    path.backlink.assign(score.size(), -1);
    path.cumulative = score;
    bool firstBeat = true;
    for (std::size_t at = 0; at < score.size(); ++at)
    {
        const auto best = bestPredecessor(path.cumulative, static_cast<std::ptrdiff_t>(at), window);
        if (best.location >= 0)
            path.cumulative[at] += best.score;
        if (firstBeat && score[at] < threshold)
            continue;
        path.backlink[at] = best.location;
        firstBeat = false;
    }
    return path;
}

} // namespace

std::ptrdiff_t lastBeat(const std::vector<double>& cumulative)
{
    const std::size_t size = cumulative.size();
    std::vector<bool> isMaximum(size);
    for (std::size_t index = 1; index + 1 < size; ++index)
        isMaximum[index] =
            cumulative[index] > cumulative[index - 1] && cumulative[index] >= cumulative[index + 1];
    isMaximum[size - 1] = cumulative[size - 1] > cumulative[size - 2];
    std::vector<double> maxima;
    for (std::size_t index = 0; index < size; ++index)
        if (isMaximum[index])
            maxima.push_back(cumulative[index]);
    if (maxima.empty())
        return static_cast<std::ptrdiff_t>(size) - 1;
    std::ranges::sort(maxima);
    const std::size_t middle = maxima.size() / 2;
    const double median =
        maxima.size() % 2 == 1 ? maxima[middle] : 0.5 * (maxima[middle - 1] + maxima[middle]);
    for (auto index = static_cast<std::ptrdiff_t>(size) - 1; index >= 0; --index)
        if (isMaximum[static_cast<std::size_t>(index)] &&
            cumulative[static_cast<std::size_t>(index)] >= 0.5 * median)
            return index;
    return static_cast<std::ptrdiff_t>(size) - 1;
}

void trimBeats(const std::vector<double>& score, std::vector<bool>& beats)
{
    std::vector<double> beatScores;
    for (std::size_t index = 0; index < score.size(); ++index)
        if (beats[index])
            beatScores.push_back(score[index]);
    // np.convolve(beatScores, hanning(5)) from index 2 on, at most score.size() values.
    constexpr std::array<double, 5> kHann{0.0, 0.5, 1.0, 0.5, 0.0};
    const std::size_t full = beatScores.size() + kHann.size() - 1;
    double squares = 0.0;
    std::size_t count = 0;
    for (std::size_t index = 2; index < full && index < score.size() + 2; ++index, ++count)
    {
        double smooth = 0.0;
        for (std::size_t tap = 0; tap < kHann.size(); ++tap)
            if (index >= tap && index - tap < beatScores.size())
                smooth += kHann.at(tap) * beatScores[index - tap];
        squares += smooth * smooth;
    }
    const double threshold = 0.5 * std::sqrt(squares / static_cast<double>(count));
    for (std::size_t index = 0; index < score.size() && score[index] <= threshold; ++index)
        beats[index] = false;
    for (std::size_t index = score.size(); index > 0 && score[index - 1] <= threshold; --index)
        beats[index - 1] = false;
}

std::vector<double> trackBeats(std::span<const float> envelope,
                               double framesPerSecond,
                               double bpm,
                               const BeatTrackerSettings& settings)
{
    // librosa needs a standard deviation: at least two values, not all zero.
    if (envelope.size() < 2 || std::ranges::none_of(envelope, [](float value) { return value != 0.0F; }))
        return {};
    const auto framesPerBeat = static_cast<std::ptrdiff_t>(std::nearbyint(framesPerSecond * 60.0 / bpm));
    const auto score = localScore(envelope, framesPerBeat);
    const auto path = dynamicProgram(score, framesPerBeat, settings);
    std::vector<bool> beats(score.size());
    for (auto frame = lastBeat(path.cumulative); frame >= 0;
         frame = path.backlink[static_cast<std::size_t>(frame)])
        beats[static_cast<std::size_t>(frame)] = true;
    trimBeats(score, beats);
    std::vector<double> seconds;
    for (std::size_t frame = 0; frame < beats.size(); ++frame)
        if (beats[frame])
            seconds.push_back(static_cast<double>(frame) / framesPerSecond);
    return seconds;
}

} // namespace snotes::rhythm
