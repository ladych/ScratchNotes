#pragma once

#include "ui/IWriterThread.h"

namespace snotes::ui
{

/// Records attach and detach; tests pump the writer themselves.
class FakeWriterThread final : public IWriterThread
{
public:
    void attach(io::TakeFileWriter& writer) override { attached = &writer; }
    void detach() override
    {
        attached = nullptr;
        ++detaches;
    }

    io::TakeFileWriter* attached = nullptr;
    int detaches = 0;
};

} // namespace snotes::ui
