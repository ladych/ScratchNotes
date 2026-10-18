#pragma once

#include "model/Project.h"

#include <juce_data_structures/juce_data_structures.h>

#include <functional>
#include <string>
#include <vector>

namespace snotes::ui
{

/// What the track view shows of one voice (F-AU-01, F-AU-04, F-WI-03; GUI-Entwurf v0.1, item 5).
struct TrackRow
{
    std::string id;
    std::string name;
    /// Index into instrumentProfileChoices(), -1 for an unknown profile.
    int profile = -1;
    /// Index into inputChoices(): 0 is "none", input n is n + 1.
    int diChoice = 0;
    int wetChoice = 0;
    bool armed = false;
    bool muted = false;
    bool solo = false;
    engine::MonitorSource monitor = engine::MonitorSource::off;
    double gain = 1.0;
    /// e.g. "Take 2 · 09:40"
    std::vector<std::string> takes;
    /// Index into takes, -1 without takes.
    int activeTake = -1;
    /// Files of the active take relative to the project folder; empty without take or signal.
    std::string diFile;
    std::string wetFile;
    /// Caption of the DI waveform, with the number of notes once they are detected (F-TX-01), e.g.
    /// "DI · 42 notes".
    std::string diCaption;
    /// Song time in seconds at which the active take plays, after latency compensation (F-WI-05).
    double takeStartSeconds = 0.0;
};

/// Seconds the track view shows from the song start: the takes and the position with some room behind,
/// in steps of 10 seconds, at least 30 seconds.
[[nodiscard]] double visibleSeconds(double contentEndSeconds, double positionSeconds);

/// Translated names of the instrument profiles, in the order of model::instrumentProfiles().
[[nodiscard]] std::vector<std::string> instrumentProfileChoices();
/// Names of the monitoring choices, in the order of engine::MonitorSource.
[[nodiscard]] std::vector<std::string> monitorChoices();

/// "— none —" followed by the device inputs, numbered from 1 like the interface labels them, e.g. "1 · Hi-Z
/// 1".
[[nodiscard]] std::vector<std::string> inputChoices(const std::vector<std::string>& inputNames);

/// The rows of the track view, `isArmed` tells whether a voice is armed for recording.
[[nodiscard]] std::vector<TrackRow> trackRows(const model::Project& project,
                                              const std::function<bool(const std::string&)>& isArmed);

/// Edits of the track view. Each one is a separate undo step (F-PJ-04); ids of removed voices are ignored.
class TrackEditor
{
public:
    explicit TrackEditor(model::Project project);

    void rename(const std::string& voiceId, const std::string& name);
    void chooseProfile(const std::string& voiceId, int profile);
    void chooseInputs(const std::string& voiceId, int diChoice, int wetChoice);
    void setMuted(const std::string& voiceId, bool muted);
    void setSolo(const std::string& voiceId, bool solo);
    void setMonitor(const std::string& voiceId, engine::MonitorSource source);
    void setGain(const std::string& voiceId, double gain);
    void chooseTake(const std::string& voiceId, int take);
    /// The audio files stay on disk, so the deletion can be undone.
    void deleteActiveTake(const std::string& voiceId);
    void removeVoice(const std::string& voiceId);
    /// Click on or off and its tempo (F-WI-07).
    void setClickOn(bool on);
    void setClickBpm(double beatsPerMinute);

private:
    void edit(const std::string& voiceId, const std::function<void(model::Voice&)>& change);

    model::Project project_;
};

} // namespace snotes::ui
