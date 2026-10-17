#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace snotes::io
{

/// A file written in pieces, e.g. a WAV file during recording, behind an interface so the writers can be
/// tested without touching disk (E-06).
class IBinaryFile
{
public:
    virtual ~IBinaryFile() = default;

    virtual bool append(std::span<const std::byte> bytes) = 0;
    /// Replaces bytes already written, starting at `offset`.
    virtual bool overwrite(std::uint64_t offset, std::span<const std::byte> bytes) = 0;
    /// Hands everything written so far to the operating system.
    virtual bool flush() = 0;

protected:
    IBinaryFile() = default;
    IBinaryFile(const IBinaryFile&) = default;
    IBinaryFile(IBinaryFile&&) = default;
    IBinaryFile& operator=(const IBinaryFile&) = default;
    IBinaryFile& operator=(IBinaryFile&&) = default;
};

} // namespace snotes::io
