#include "ui/VoiceList.h"

#include "i18n/Translate.h"
#include "model/InstrumentProfile.h"

#include <set>
#include <string>

namespace snotes::ui
{

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

} // namespace snotes::ui
