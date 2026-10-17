#include "ui/VoiceList.h"

#include "i18n/Translate.h"
#include "model/InstrumentProfile.h"

#include <set>
#include <string>

namespace snotes::ui
{
namespace
{

std::string inputText(const model::Voice& voice)
{
    if (voice.inputDi() == model::kNoInput)
        return i18n::tr("no input");
    const auto di = std::to_string(voice.inputDi() + 1);
    if (voice.inputWet() == model::kNoInput)
        return i18n::tr("Input %1", di);
    return i18n::tr("Inputs %1 (DI) + %2", di, std::to_string(voice.inputWet() + 1));
}

std::string profileText(const model::Voice& voice)
{
    const auto profile = model::findInstrumentProfile(voice.profileId());
    return profile ? i18n::tr(profile->name.c_str()) : voice.profileId();
}

} // namespace

model::Voice addVoice(model::Project& project, model::IIdGenerator& idGenerator, int numInputs)
{
    std::set<int> used;
    for (int index = 0; index < project.numVoices(); ++index)
    {
        used.insert(project.voice(index).inputDi());
        used.insert(project.voice(index).inputWet());
    }
    int input = 0;
    while (input < numInputs && used.contains(input))
        ++input;

    project.undoManager()->beginNewTransaction();
    return project.addVoice({.id = idGenerator.next(),
                             .name = i18n::tr("Voice %1", std::to_string(project.numVoices() + 1)),
                             .profileId = model::defaultInstrumentProfile().id,
                             .inputDi = input < numInputs ? input : model::kNoInput,
                             .inputWet = model::kNoInput});
}

std::vector<VoiceEntry> voiceEntries(const model::Project& project)
{
    std::vector<VoiceEntry> entries;
    for (int index = 0; index < project.numVoices(); ++index)
    {
        const auto voice = project.voice(index);
        entries.push_back({voice.name(), profileText(voice) + " · " + inputText(voice)});
    }
    return entries;
}

} // namespace snotes::ui
