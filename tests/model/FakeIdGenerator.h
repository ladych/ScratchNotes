#pragma once

#include "model/IIdGenerator.h"

#include <string>

namespace snotes::model
{

/// Predictable ids "id-1", "id-2", ...
class FakeIdGenerator final : public IIdGenerator
{
public:
    [[nodiscard]] std::string next() override { return "id-" + std::to_string(++count_); }

private:
    int count_ = 0;
};

} // namespace snotes::model
