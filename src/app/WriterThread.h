#pragma once

#include "ui/IWriterThread.h"

#include <juce_core/juce_core.h>

#include <mutex>

namespace snotes::app
{

/// Humble object (E-03): the thread that pumps the attached io::TakeFileWriter every few milliseconds, so
/// the audio thread never touches a file (Q-04).
class WriterThread final : public ui::IWriterThread, private juce::Thread
{
public:
    WriterThread();
    ~WriterThread() override;

    WriterThread(const WriterThread&) = delete;
    WriterThread& operator=(const WriterThread&) = delete;
    WriterThread(WriterThread&&) = delete;
    WriterThread& operator=(WriterThread&&) = delete;

    void attach(io::TakeFileWriter& writer) override;
    void detach() override;

private:
    void run() override;

    std::mutex mutex_;
    io::TakeFileWriter* writer_ = nullptr;
};

} // namespace snotes::app
