#include "model/UuidGenerator.h"

#include <juce_core/juce_core.h>

namespace snotes::model
{

std::string UuidGenerator::next()
{
    return juce::Uuid{}.toDashedString().toStdString();
}

} // namespace snotes::model
