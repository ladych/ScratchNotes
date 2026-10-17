#include "ui/TrackList.h"

#include "i18n/Translate.h"
#include "model/InstrumentProfile.h"

#include <utility>

namespace snotes::ui
{
namespace
{

/// "2026-10-17T09:40:00" -> "09:40"
std::string clockTime(const std::string& recordedAt)
{
    constexpr std::size_t kTimeStart = 11;
    constexpr std::size_t kTimeLength = 5;
    return recordedAt.size() >= kTimeStart + kTimeLength ? recordedAt.substr(kTimeStart, kTimeLength)
                                                         : recordedAt;
}

int profileIndex(const std::string& profileId)
{
    const auto& profiles = model::instrumentProfiles();
    for (std::size_t index = 0; index < profiles.size(); ++index)
        if (profiles[index].id == profileId)
            return static_cast<int>(index);
    return -1;
}

void fillTakes(TrackRow& row, const model::Voice& voice)
{
    for (int number = 0; number < voice.numTakes(); ++number)
    {
        const auto take = voice.take(number);
        auto label = i18n::tr("Take %1 · %2", std::to_string(number + 1), clockTime(take.recordedAt()));
        if (take.isAudioMissing())
            label += i18n::tr(" (missing)");
        row.takes.push_back(std::move(label));
    }
    row.activeTake = voice.activeTakeIndex();
    if (row.activeTake >= 0)
    {
        const auto active = voice.take(row.activeTake);
        row.diFile = active.diFile();
        row.wetFile = active.wetFile();
    }
}

} // namespace

std::vector<std::string> instrumentProfileChoices()
{
    std::vector<std::string> choices;
    for (const auto& profile : model::instrumentProfiles())
        choices.push_back(i18n::tr(profile.name.c_str()));
    return choices;
}

std::vector<std::string> inputChoices(const std::vector<std::string>& inputNames)
{
    std::vector<std::string> choices;
    choices.push_back(i18n::tr("— none —"));
    for (std::size_t index = 0; index < inputNames.size(); ++index)
        choices.push_back(std::to_string(index + 1) + " · " + inputNames[index]);
    return choices;
}

std::vector<TrackRow> trackRows(const model::Project& project,
                                const std::function<bool(const std::string&)>& isArmed)
{
    std::vector<TrackRow> rows;
    for (int index = 0; index < project.numVoices(); ++index)
    {
        const auto voice = project.voice(index);
        TrackRow row;
        row.id = voice.id();
        row.name = voice.name();
        row.profile = profileIndex(voice.profileId());
        row.diChoice = voice.inputDi() + 1;
        row.wetChoice = voice.inputWet() + 1;
        row.armed = isArmed(row.id);
        row.muted = voice.isMuted();
        row.solo = voice.isSolo();
        row.gain = voice.gain();
        fillTakes(row, voice);
        rows.push_back(std::move(row));
    }
    return rows;
}

TrackEditor::TrackEditor(model::Project project) : project_(std::move(project)) {}

void TrackEditor::rename(const std::string& voiceId, const std::string& name)
{
    edit(voiceId, [&name](model::Voice& voice) { voice.setName(name); });
}

void TrackEditor::chooseProfile(const std::string& voiceId, int profile)
{
    const auto& profiles = model::instrumentProfiles();
    if (profile < 0 || static_cast<std::size_t>(profile) >= profiles.size())
        return;
    edit(voiceId,
         [&profiles, profile](model::Voice& voice)
         { voice.setProfileId(profiles[static_cast<std::size_t>(profile)].id); });
}

void TrackEditor::chooseInputs(const std::string& voiceId, int diChoice, int wetChoice)
{
    edit(voiceId,
         [diChoice, wetChoice](model::Voice& voice)
         {
             voice.setInputDi(diChoice - 1);
             voice.setInputWet(wetChoice - 1);
         });
}

void TrackEditor::setMuted(const std::string& voiceId, bool muted)
{
    edit(voiceId, [muted](model::Voice& voice) { voice.setMuted(muted); });
}

void TrackEditor::setSolo(const std::string& voiceId, bool solo)
{
    edit(voiceId, [solo](model::Voice& voice) { voice.setSolo(solo); });
}

void TrackEditor::setGain(const std::string& voiceId, double gain)
{
    edit(voiceId, [gain](model::Voice& voice) { voice.setGain(gain); });
}

void TrackEditor::chooseTake(const std::string& voiceId, int take)
{
    edit(voiceId,
         [take](model::Voice& voice)
         {
             if (take >= 0 && take < voice.numTakes())
                 voice.setActiveTake(voice.take(take).id());
         });
}

void TrackEditor::deleteActiveTake(const std::string& voiceId)
{
    auto voice = project_.findVoice(voiceId);
    if (!voice)
        return;
    const int index = voice->activeTakeIndex();
    if (index < 0)
        return;
    project_.undoManager()->beginNewTransaction();
    voice->removeTake(index);
}

void TrackEditor::removeVoice(const std::string& voiceId)
{
    for (int index = 0; index < project_.numVoices(); ++index)
        if (project_.voice(index).id() == voiceId)
        {
            project_.undoManager()->beginNewTransaction();
            project_.removeVoice(index);
            return;
        }
}

void TrackEditor::edit(const std::string& voiceId, const std::function<void(model::Voice&)>& change)
{
    auto voice = project_.findVoice(voiceId);
    if (!voice)
        return;
    project_.undoManager()->beginNewTransaction();
    change(*voice);
}

} // namespace snotes::ui
