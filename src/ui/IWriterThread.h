#pragma once

#include "io/TakeFileWriter.h"

namespace snotes::ui
{

/// The thread that writes the recorded audio to disk (Pflichtenheft chapter 3): it pumps the attached
/// writer regularly until it is detached.
class IWriterThread
{
public:
    virtual ~IWriterThread() = default;

    virtual void attach(io::TakeFileWriter& writer) = 0;
    /// Returns once the thread no longer touches the writer.
    virtual void detach() = 0;

protected:
    IWriterThread() = default;
    IWriterThread(const IWriterThread&) = default;
    IWriterThread(IWriterThread&&) = default;
    IWriterThread& operator=(const IWriterThread&) = default;
    IWriterThread& operator=(IWriterThread&&) = default;
};

} // namespace snotes::ui
