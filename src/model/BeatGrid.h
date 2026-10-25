#pragma once

#include "model/TreeNode.h"

#include <cstdint>
#include <string>
#include <vector>

namespace snotes::model
{

/// A meter the analysis suggests (F-TX-06).
struct MeterReading
{
    /// Beats per bar, 3 to 7.
    int numerator = 4;
    /// 0..1, all readings add up to 1.
    double confidence = 0.0;
    /// Index of the first beat that starts a bar.
    int phase = 0;

    bool operator==(const MeterReading&) const = default;
};

/// The beat grid all voices share (F-TX-05), with the meter suggestions: BEAT_GRID with the beats as sample
/// positions of the song, separated by spaces, and one METER_READING per suggestion, most likely first.
/// Not undoable: it is the result of an analysis, like the notes.
class BeatGrid : public TreeNode
{
public:
    explicit BeatGrid(juce::ValueTree tree);

    static constexpr const char* kAnalysed = "auto";
    /// The user chose the meter or moved, inserted or deleted beats (F-KO-01, F-KO-02).
    static constexpr const char* kEdited = "edited";

    [[nodiscard]] std::vector<std::int64_t> beats() const;
    /// kAnalysed after the analysis, kEdited after a correction; empty without a grid.
    [[nodiscard]] std::string source() const;
    [[nodiscard]] bool isEdited() const { return source() == kEdited; }
    [[nodiscard]] std::vector<MeterReading> meterReadings() const;
    /// Beats per bar of the chosen meter, 0 without a grid.
    [[nodiscard]] int chosenMeter() const;
    [[nodiscard]] int downbeatPhase() const;

    /// The score of all voices in the chosen meter (F-NA-02), a MusicXML file relative to the project folder;
    /// empty without a grid or without notes.
    [[nodiscard]] std::string scoreFile() const;
    void setScoreFile(const std::string& file);
    /// The beat of the grid on which the score starts, counted from 0; the cursor in the score follows the
    /// song from there (F-NA-04).
    [[nodiscard]] int scoreOrigin() const;
    void setScoreOrigin(int beat);

    /// Stores an analysis; the most likely meter becomes the chosen one.
    void setAnalysis(const std::vector<std::int64_t>& beats, const std::vector<MeterReading>& readings);
    /// A correction: the beats, the meter and the first downbeat become the user's, the readings of the
    /// analysis and the score stay.
    void setEdited(const std::vector<std::int64_t>& beats, int meter, int phase);
    /// Also forgets the score.
    void clear();
};

} // namespace snotes::model
