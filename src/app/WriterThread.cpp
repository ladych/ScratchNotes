#include "app/WriterThread.h"

namespace snotes::app
{
namespace
{

constexpr int kIntervalMs = 10;
constexpr int kStopTimeoutMs = 2000;

} // namespace

WriterThread::WriterThread() : juce::Thread("Scratch Notes writer")
{
    startThread();
}

WriterThread::~WriterThread()
{
    stopThread(kStopTimeoutMs);
}

void WriterThread::attach(io::TakeFileWriter& writer)
{
    const std::scoped_lock lock{mutex_};
    writer_ = &writer;
}

void WriterThread::detach()
{
    const std::scoped_lock lock{mutex_};
    writer_ = nullptr;
}

void WriterThread::run()
{
    while (!threadShouldExit())
    {
        {
            const std::scoped_lock lock{mutex_};
            if (writer_ != nullptr)
                writer_->pump();
        }
        wait(kIntervalMs);
    }
}

} // namespace snotes::app
