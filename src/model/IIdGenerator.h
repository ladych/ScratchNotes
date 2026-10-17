#pragma once

#include <string>

namespace snotes::model
{

/// Creates unique ids for voices and takes. Behind an interface so tests get predictable ids (E-04).
class IIdGenerator
{
public:
    virtual ~IIdGenerator() = default;

    [[nodiscard]] virtual std::string next() = 0;

protected:
    IIdGenerator() = default;
    IIdGenerator(const IIdGenerator&) = default;
    IIdGenerator(IIdGenerator&&) = default;
    IIdGenerator& operator=(const IIdGenerator&) = default;
    IIdGenerator& operator=(IIdGenerator&&) = default;
};

} // namespace snotes::model
