#pragma once

#include "io/IBinaryFile.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <vector>

namespace snotes::io
{

/// What a FakeBinaryFile wrote; shared, so tests can read it after the writer took the file.
struct FakeFileContent
{
    std::vector<std::byte> bytes;
    int flushes = 0;
    /// The write with this number (counting appends, overwrites and flushes from 1) fails; 0: none.
    int failAtCall = 0;
    int calls = 0;

    [[nodiscard]] std::uint32_t uint32At(std::size_t offset) const
    {
        std::uint32_t value = 0;
        std::memcpy(&value, &bytes.at(offset), sizeof(value));
        return value;
    }

    [[nodiscard]] float floatAt(std::size_t offset) const
    {
        float value = 0.0F;
        std::memcpy(&value, &bytes.at(offset), sizeof(value));
        return value;
    }
};

class FakeBinaryFile final : public IBinaryFile
{
public:
    explicit FakeBinaryFile(std::shared_ptr<FakeFileContent> content) : content_(std::move(content)) {}

    bool append(std::span<const std::byte> bytes) override
    {
        if (fails())
            return false;
        content_->bytes.insert(content_->bytes.end(), bytes.begin(), bytes.end());
        return true;
    }

    bool overwrite(std::uint64_t offset, std::span<const std::byte> bytes) override
    {
        if (fails())
            return false;
        std::ranges::copy(bytes, content_->bytes.begin() + static_cast<std::ptrdiff_t>(offset));
        return true;
    }

    bool flush() override
    {
        if (fails())
            return false;
        ++content_->flushes;
        return true;
    }

private:
    bool fails() { return ++content_->calls == content_->failAtCall; }

    std::shared_ptr<FakeFileContent> content_;
};

} // namespace snotes::io
