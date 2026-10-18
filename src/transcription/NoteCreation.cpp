#include "transcription/NoteCreation.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace snotes::transcription
{
namespace
{

using basicpitch::kNumPitches;
constexpr int kMaxPitchIndex = kNumPitches - 1;
/// get_infered_onsets compares each frame with the two frames before it.
constexpr int kOnsetDiffFrames = 2;

std::size_t at(int frame, int pitch)
{
    return Activations::index(frame, pitch);
}

/// Smallest rise of the note activation over the last one and two frames, never below zero.
double frameRise(const Activations& activations, int frame, int pitch)
{
    if (frame < kOnsetDiffFrames)
        return 0.0;
    double rise = std::numeric_limits<double>::infinity();
    for (int back = 1; back <= kOnsetDiffFrames; ++back)
    {
        const double before = activations.frame(frame - back, pitch);
        rise = std::min(rise, static_cast<double>(activations.frame(frame, pitch)) - before);
    }
    return std::max(rise, 0.0);
}

/// get_infered_onsets: the onsets, or the rises of the note activation scaled to the same maximum.
std::vector<double> inferredOnsets(const Activations& activations)
{
    std::vector<double> rises(activations.frames.size());
    for (int frame = 0; frame < activations.numFrames; ++frame)
        for (int pitch = 0; pitch < kNumPitches; ++pitch)
            rises[at(frame, pitch)] = frameRise(activations, frame, pitch);
    const double maxRise = *std::ranges::max_element(rises);
    const double maxOnset = *std::ranges::max_element(activations.onsets);
    std::vector<double> onsets(activations.onsets.begin(), activations.onsets.end());
    // Python divides 0 by 0 here and gets no onsets at all.
    if (maxRise <= 0.0)
        return std::vector<double>(onsets.size(), 0.0);
    for (std::size_t cell = 0; cell < onsets.size(); ++cell)
        onsets[cell] = std::max(onsets[cell], maxOnset * rises[cell] / maxRise);
    return onsets;
}

/// Onset peaks (scipy.signal.argrelmax over time) at or above the threshold, latest first.
std::vector<std::pair<int, int>>
onsetPeaks(const std::vector<double>& onsets, int numFrames, double threshold)
{
    std::vector<std::pair<int, int>> peaks;
    for (int frame = 1; frame + 1 < numFrames; ++frame)
        for (int pitch = 0; pitch < kNumPitches; ++pitch)
        {
            const double value = onsets[at(frame, pitch)];
            if (value > onsets[at(frame - 1, pitch)] && value > onsets[at(frame + 1, pitch)] &&
                value >= threshold)
                peaks.emplace_back(frame, pitch);
        }
    std::ranges::reverse(peaks);
    return peaks;
}

class NoteDecoder
{
public:
    NoteDecoder(const Activations& activations, const NoteSettings& settings)
        : activations_(activations), settings_(settings),
          energy_(activations.frames.begin(), activations.frames.end())
    {
    }

    std::vector<FrameNote> decode()
    {
        const auto onsets = onsetValues();
        for (const auto& [frame, pitch] : onsetPeaks(onsets, frames(), settings_.onsetThreshold))
            addOnsetNote(frame, pitch);
        if (settings_.melodiaTrick)
            addMelodiaNotes();
        return std::move(notes_);
    }

private:
    [[nodiscard]] int frames() const { return activations_.numFrames; }

    [[nodiscard]] std::vector<double> onsetValues() const
    {
        if (settings_.inferOnsets)
            return inferredOnsets(activations_);
        return {activations_.onsets.begin(), activations_.onsets.end()};
    }

    [[nodiscard]] bool isQuiet(int frame, int pitch) const
    {
        return energy_[at(frame, pitch)] < settings_.frameThreshold;
    }

    void clear(int frame, int pitch)
    {
        energy_[at(frame, pitch)] = 0.0;
        if (pitch < kMaxPitchIndex)
            energy_[at(frame, pitch + 1)] = 0.0;
        if (pitch > 0)
            energy_[at(frame, pitch - 1)] = 0.0;
    }

    /// Peaks lie inside the frames, so start + 1 is a frame (Basic Pitch also checks it, needlessly).
    void addOnsetNote(int start, int pitch)
    {
        int frame = start + 1;
        int quiet = 0;
        while (frame < frames() - 1 && quiet < settings_.energyTolerance)
        {
            quiet = isQuiet(frame, pitch) ? quiet + 1 : 0;
            ++frame;
        }
        const int end = frame - quiet;
        if (end - start <= settings_.minimumFrames)
            return;
        for (int cleared = start; cleared < end; ++cleared)
            clear(cleared, pitch);
        add(start, end, pitch);
    }

    void addMelodiaNotes()
    {
        while (true)
        {
            const auto peak = std::ranges::max_element(energy_);
            if (*peak <= settings_.frameThreshold)
                return;
            const auto cell = static_cast<int>(peak - energy_.begin());
            addMelodiaNote(cell / kNumPitches, cell % kNumPitches);
        }
    }

    void addMelodiaNote(int middle, int pitch)
    {
        energy_[at(middle, pitch)] = 0.0;
        const int end = followForward(middle, pitch);
        const int start = followBackward(middle, pitch);
        if (end - start <= settings_.minimumFrames)
            return;
        add(start, end, pitch);
    }

    int followForward(int middle, int pitch)
    {
        int frame = middle + 1;
        int quiet = 0;
        while (frame < frames() - 1 && quiet < settings_.energyTolerance)
        {
            quiet = isQuiet(frame, pitch) ? quiet + 1 : 0;
            clear(frame, pitch);
            ++frame;
        }
        return frame - 1 - quiet;
    }

    int followBackward(int middle, int pitch)
    {
        int frame = middle - 1;
        int quiet = 0;
        while (frame > 0 && quiet < settings_.energyTolerance)
        {
            quiet = isQuiet(frame, pitch) ? quiet + 1 : 0;
            clear(frame, pitch);
            --frame;
        }
        return frame + 1 + quiet;
    }

    void add(int start, int end, int pitch)
    {
        double sum = 0.0;
        for (int frame = start; frame < end; ++frame)
            sum += activations_.frame(frame, pitch);
        const auto amplitude = static_cast<float>(sum / (end - start));
        notes_.push_back({.startFrame = start,
                          .endFrame = end,
                          .pitch = pitch + basicpitch::kLowestPitch,
                          .amplitude = amplitude});
    }

    const Activations& activations_;
    const NoteSettings& settings_;
    /// remaining_energy in Python: activation that no note has taken yet.
    std::vector<double> energy_;
    std::vector<FrameNote> notes_;
};

} // namespace

std::vector<FrameNote> createNotes(const Activations& activations, const NoteSettings& settings)
{
    if (activations.numFrames == 0)
        return {};
    return NoteDecoder{activations, settings}.decode();
}

int minimumFramesFor(double milliseconds)
{
    return static_cast<int>(
        std::lround(milliseconds / 1000.0 * (basicpitch::kSampleRate / basicpitch::kFftHop)));
}

double frameToSeconds(int frame)
{
    constexpr double kHopSeconds = basicpitch::kFftHop / basicpitch::kSampleRate;
    // The magic 0.0018 is Basic Pitch's own, needed to align the windows.
    constexpr double kWindowOffset =
        (kHopSeconds * (basicpitch::kFramesPerWindow -
                        (static_cast<double>(basicpitch::kWindowSamples) / basicpitch::kFftHop))) +
        0.0018;
    const int window = frame / basicpitch::kFramesPerWindow;
    return (frame * kHopSeconds) - (kWindowOffset * window);
}

} // namespace snotes::transcription
