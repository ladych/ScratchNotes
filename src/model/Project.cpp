#include "model/Project.h"

#include "model/ModelIds.h"

#include <algorithm>
#include <utility>

namespace snotes::model
{
namespace
{

constexpr double kMaxGain = 4.0;

juce::String toJuce(const std::string& utf8)
{
    return juce::String::fromUTF8(utf8.c_str());
}

double clampGain(double gain)
{
    return std::clamp(gain, 0.0, kMaxGain);
}

} // namespace

Take::Take(juce::ValueTree tree, juce::UndoManager* undoManager) : TreeNode(std::move(tree), undoManager) {}

TakeData Take::data() const
{
    return {.id = id(),
            .diFile = diFile(),
            .wetFile = wetFile(),
            .startSample = startSample(),
            .latencyOffsetSamples = latencyOffsetSamples(),
            .recordedAt = recordedAt()};
}

std::string Take::id() const
{
    return getString(ids::id);
}

std::string Take::diFile() const
{
    return getString(ids::diFile);
}

std::string Take::wetFile() const
{
    return getString(ids::wetFile);
}

std::int64_t Take::startSample() const
{
    return getInt64(ids::startSample, 0);
}

std::int64_t Take::latencyOffsetSamples() const
{
    return getInt64(ids::latencyOffsetSamples, 0);
}

std::string Take::recordedAt() const
{
    return getString(ids::recordedAt);
}

bool Take::isAudioMissing() const
{
    return static_cast<bool>(tree().getProperty(ids::audioMissing, false));
}

void Take::setStartSample(std::int64_t startSample)
{
    set(ids::startSample, static_cast<juce::int64>(startSample));
}

void Take::setLatencyOffsetSamples(std::int64_t latencyOffsetSamples)
{
    set(ids::latencyOffsetSamples, static_cast<juce::int64>(latencyOffsetSamples));
}

Voice::Voice(juce::ValueTree tree, juce::UndoManager* undoManager) : TreeNode(std::move(tree), undoManager) {}

std::string Voice::id() const
{
    return getString(ids::id);
}

std::string Voice::name() const
{
    return getString(ids::name);
}

std::string Voice::profileId() const
{
    return getString(ids::profileId);
}

int Voice::inputDi() const
{
    return getInt(ids::inputDi, kNoInput);
}

int Voice::inputWet() const
{
    return getInt(ids::inputWet, kNoInput);
}

double Voice::gain() const
{
    return getDouble(ids::gain, 1.0);
}

bool Voice::isMuted() const
{
    return getInt(ids::mute, 0) != 0;
}

bool Voice::isSolo() const
{
    return getInt(ids::solo, 0) != 0;
}

engine::MonitorSource Voice::monitor() const
{
    // Older projects stored 1 for "monitored", which meant the processed input.
    switch (getInt(ids::monitor, 0))
    {
    case 1:
        return engine::MonitorSource::processed;
    case 2:
        return engine::MonitorSource::di;
    default:
        return engine::MonitorSource::off;
    }
}

void Voice::setName(const std::string& name)
{
    setString(ids::name, name);
}

void Voice::setProfileId(const std::string& profileId)
{
    setString(ids::profileId, profileId);
}

void Voice::setInputDi(int input)
{
    set(ids::inputDi, input);
}

void Voice::setInputWet(int input)
{
    set(ids::inputWet, input);
}

void Voice::setGain(double gain)
{
    set(ids::gain, clampGain(gain));
}

void Voice::setMuted(bool muted)
{
    set(ids::mute, muted ? 1 : 0);
}

void Voice::setSolo(bool solo)
{
    set(ids::solo, solo ? 1 : 0);
}

void Voice::setMonitor(engine::MonitorSource source)
{
    set(ids::monitor, static_cast<int>(source));
}

int Voice::numTakes() const
{
    return tree().getChildWithName(ids::takes).getNumChildren();
}

Take Voice::take(int index) const
{
    return {tree().getChildWithName(ids::takes).getChild(index), undoManager()};
}

std::optional<Take> Voice::findTake(const std::string& id) const
{
    auto found = tree().getChildWithName(ids::takes).getChildWithProperty(ids::id, toJuce(id));
    if (!found.isValid())
        return std::nullopt;
    return Take{found, undoManager()};
}

std::optional<Take> Voice::activeTake() const
{
    return findTake(getString(ids::activeTakeId));
}

int Voice::activeTakeIndex() const
{
    const auto takes = tree().getChildWithName(ids::takes);
    return takes.indexOf(takes.getChildWithProperty(ids::id, tree()[ids::activeTakeId]));
}

Take Voice::addTake(const TakeData& data)
{
    juce::ValueTree take{ids::take};
    take.setProperty(ids::id, toJuce(data.id), nullptr);
    take.setProperty(ids::diFile, toJuce(data.diFile), nullptr);
    if (!data.wetFile.empty())
        take.setProperty(ids::wetFile, toJuce(data.wetFile), nullptr);
    take.setProperty(ids::startSample, static_cast<juce::int64>(data.startSample), nullptr);
    take.setProperty(ids::latencyOffsetSamples, static_cast<juce::int64>(data.latencyOffsetSamples), nullptr);
    take.setProperty(ids::recordedAt, toJuce(data.recordedAt), nullptr);
    mutableTree().getOrCreateChildWithName(ids::takes, undoManager()).appendChild(take, undoManager());
    setString(ids::activeTakeId, data.id);
    return {take, undoManager()};
}

void Voice::removeTake(int index)
{
    auto takes = mutableTree().getChildWithName(ids::takes);
    const bool wasActive = takes.getChild(index)[ids::id].toString() == tree()[ids::activeTakeId].toString();
    takes.removeChild(index, undoManager());
    if (!wasActive)
        return;
    const auto last = takes.getChild(takes.getNumChildren() - 1);
    set(ids::activeTakeId, last[ids::id]);
}

void Voice::setActiveTake(const std::string& id)
{
    if (findTake(id))
        setString(ids::activeTakeId, id);
}

Mix::Mix(juce::ValueTree tree, juce::UndoManager* undoManager) : TreeNode(std::move(tree), undoManager) {}

double Mix::masterGain() const
{
    return getDouble(ids::masterGain, 1.0);
}

void Mix::setMasterGain(double gain)
{
    set(ids::masterGain, clampGain(gain));
}

bool Mix::isClickOn() const
{
    return getInt(ids::click, 0) != 0;
}

double Mix::clickBpm() const
{
    return getDouble(ids::clickBpm, kDefaultClickBpm);
}

void Mix::setClickOn(bool on)
{
    set(ids::click, on ? 1 : 0);
}

void Mix::setClickBpm(double beatsPerMinute)
{
    set(ids::clickBpm, std::clamp(beatsPerMinute, kMinClickBpm, kMaxClickBpm));
}

Project::Project(juce::ValueTree tree, juce::UndoManager* undoManager)
    : TreeNode(std::move(tree), undoManager)
{
}

std::string Project::name() const
{
    return getString(ids::name);
}

double Project::sampleRate() const
{
    return getDouble(ids::sampleRate, 0.0);
}

void Project::setName(const std::string& name)
{
    setString(ids::name, name);
}

int Project::numVoices() const
{
    return child(ids::voices).getNumChildren();
}

Voice Project::voice(int index) const
{
    return {child(ids::voices).getChild(index), undoManager()};
}

std::optional<Voice> Project::findVoice(const std::string& id) const
{
    auto found = child(ids::voices).getChildWithProperty(ids::id, toJuce(id));
    if (!found.isValid())
        return std::nullopt;
    return Voice{found, undoManager()};
}

Voice Project::addVoice(const VoiceData& data)
{
    juce::ValueTree voice{ids::voice};
    voice.setProperty(ids::id, toJuce(data.id), nullptr);
    voice.setProperty(ids::name, toJuce(data.name), nullptr);
    voice.setProperty(ids::profileId, toJuce(data.profileId), nullptr);
    voice.setProperty(ids::inputDi, data.inputDi, nullptr);
    voice.setProperty(ids::inputWet, data.inputWet, nullptr);
    voice.appendChild(juce::ValueTree{ids::takes}, nullptr);
    mutableTree().getOrCreateChildWithName(ids::voices, undoManager()).appendChild(voice, undoManager());
    return {voice, undoManager()};
}

void Project::removeVoice(int index)
{
    mutableTree().getChildWithName(ids::voices).removeChild(index, undoManager());
}

Mix Project::mix() const
{
    return {child(ids::mix), undoManager()};
}

juce::ValueTree Project::child(const juce::Identifier& type) const
{
    return tree().getChildWithName(type);
}

} // namespace snotes::model
