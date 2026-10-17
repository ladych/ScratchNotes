#pragma once

#include "model/IIdGenerator.h"

namespace snotes::model
{

/// Random UUIDs in the form xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.
class UuidGenerator final : public IIdGenerator
{
public:
    [[nodiscard]] std::string next() override;
};

} // namespace snotes::model
