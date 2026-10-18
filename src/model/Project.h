#pragma once

#include "engine/MonitorSource.h"
#include "model/TreeNode.h"

#include <cstdint>
#include <optional>
#include <string>

namespace snotes::model
{

/// Input channel index for "no input" (a voice without processed signal).
inline constexpr int kNoInput = -1;

struct TakeData
{
    std::string id;
    /// Paths relative to the project folder with forward slashes, e.g. "audio/take-1-di.wav".
    std::string diFile;
    /// Empty if the voice records only the DI signal.
    std::string wetFile;
    std::int64_t startSample = 0;
    std::int64_t latencyOffsetSamples = 0;
    /// ISO 8601 local time, e.g. "2026-10-17T09:40:00".
    std::string recordedAt;
};

/// One pass of recording of a voice (F-AU-04).
class Take : public TreeNode
{
public:
    Take(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] TakeData data() const;
    [[nodiscard]] std::string id() const;
    [[nodiscard]] std::string diFile() const;
    [[nodiscard]] std::string wetFile() const;
    [[nodiscard]] std::int64_t startSample() const;
    [[nodiscard]] std::int64_t latencyOffsetSamples() const;
    [[nodiscard]] std::string recordedAt() const;
    /// True if one of its audio files was missing when the project was loaded (F-PJ-03).
    [[nodiscard]] bool isAudioMissing() const;
    /// Notes detected in the DI file (F-TX-01), relative to the project folder; empty before transcription.
    [[nodiscard]] std::string notesFile() const;
    [[nodiscard]] int noteCount() const;
    [[nodiscard]] bool hasNotes() const;

    void setStartSample(std::int64_t startSample);
    void setLatencyOffsetSamples(std::int64_t latencyOffsetSamples);
    /// Stores the result of a transcription. Not undoable: the file stays written either way.
    void setNotes(const std::string& file, int count);
    void clearNotes();
};

struct VoiceData
{
    std::string id;
    std::string name;
    std::string profileId;
    int inputDi = kNoInput;
    int inputWet = kNoInput;
};

/// One instrument with its inputs, mix settings and takes (F-AU-01, F-WI-03).
class Voice : public TreeNode
{
public:
    Voice(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] std::string id() const;
    [[nodiscard]] std::string name() const;
    [[nodiscard]] std::string profileId() const;
    /// Input channel of the DI signal, kNoInput if none is assigned yet.
    [[nodiscard]] int inputDi() const;
    /// Input channel of the processed signal, kNoInput if the voice records only DI.
    [[nodiscard]] int inputWet() const;
    /// Linear gain, 1 = unchanged.
    [[nodiscard]] double gain() const;
    [[nodiscard]] bool isMuted() const;
    [[nodiscard]] bool isSolo() const;
    [[nodiscard]] engine::MonitorSource monitor() const;

    void setName(const std::string& name);
    void setProfileId(const std::string& profileId);
    void setInputDi(int input);
    void setInputWet(int input);
    /// Clamped to 0 ... 4 (+12 dB).
    void setGain(double gain);
    void setMuted(bool muted);
    void setSolo(bool solo);
    void setMonitor(engine::MonitorSource source);

    [[nodiscard]] int numTakes() const;
    [[nodiscard]] Take take(int index) const;
    [[nodiscard]] std::optional<Take> findTake(const std::string& id) const;
    /// The take that plays and is transcribed; none while the voice has no takes.
    [[nodiscard]] std::optional<Take> activeTake() const;
    /// Index of activeTake(), -1 if there is none.
    [[nodiscard]] int activeTakeIndex() const;
    /// Index of the take with this id, -1 if the voice has none.
    [[nodiscard]] int takeIndex(const std::string& id) const;
    /// The new take becomes the active one.
    Take addTake(const TakeData& data);
    /// If the active take is removed, the last remaining take becomes active.
    void removeTake(int index);
    /// Ignores ids of takes the voice does not have.
    void setActiveTake(const std::string& id);
};

/// Settings of the whole mix.
class Mix : public TreeNode
{
public:
    Mix(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] double masterGain() const;
    /// Clamped like Voice::setGain.
    void setMasterGain(double gain);

    static constexpr double kDefaultClickBpm = 120.0;
    static constexpr double kMinClickBpm = 30.0;
    static constexpr double kMaxClickBpm = 300.0;

    /// Click with a fixed tempo (F-WI-07), off by default.
    [[nodiscard]] bool isClickOn() const;
    [[nodiscard]] double clickBpm() const;
    void setClickOn(bool on);
    /// Clamped to kMinClickBpm..kMaxClickBpm.
    void setClickBpm(double beatsPerMinute);
};

/// Root of the model, saved as projekt.snotes in the project folder.
class Project : public TreeNode
{
public:
    Project(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] std::string name() const;
    [[nodiscard]] double sampleRate() const;
    void setName(const std::string& name);

    [[nodiscard]] int numVoices() const;
    [[nodiscard]] Voice voice(int index) const;
    [[nodiscard]] std::optional<Voice> findVoice(const std::string& id) const;
    Voice addVoice(const VoiceData& data);
    void removeVoice(int index);

    [[nodiscard]] Mix mix() const;

private:
    [[nodiscard]] juce::ValueTree child(const juce::Identifier& type) const;
};

} // namespace snotes::model
