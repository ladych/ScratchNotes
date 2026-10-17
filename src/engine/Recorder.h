#pragma once

#include "engine/RecordingSession.h"

#include <atomic>

namespace snotes::engine
{

/// The GUI thread's view of the recorder, so the RecordingController can be tested with a recorder whose
/// audio thread is busy (E-06).
class IRecorderControl
{
public:
    virtual ~IRecorderControl() = default;

    /// Recording starts with the next audio block. The session must outlive isReleased().
    virtual void start(RecordingSession& session) noexcept = 0;
    /// Recording stops after the current audio block.
    virtual void stop() noexcept = 0;
    /// True once the audio thread no longer writes into `session`.
    [[nodiscard]] virtual bool isReleased(const RecordingSession& session) const noexcept = 0;

protected:
    IRecorderControl() = default;
    IRecorderControl(const IRecorderControl&) = default;
    IRecorderControl(IRecorderControl&&) = default;
    IRecorderControl& operator=(const IRecorderControl&) = default;
    IRecorderControl& operator=(IRecorderControl&&) = default;
};

/// Hands a RecordingSession to the audio thread and takes it back (F-AU-02). Lock-free: the audio thread
/// announces the session it is filling (a hazard pointer, as in SnapshotExchange), so the GUI thread knows
/// when a stopped session is no longer touched and its rings can be drained and freed.
///
/// AtomicPointer is std::atomic<RecordingSession*> in production; tests inject a type that simulates a
/// stop() racing with process() (E-07).
template <typename AtomicPointer = std::atomic<RecordingSession*>>
class BasicRecorder final : public IRecorderControl
{
public:
    void start(RecordingSession& session) noexcept override { session_.store(&session); }
    void stop() noexcept override { session_.store(nullptr); }
    [[nodiscard]] bool isReleased(const RecordingSession& session) const noexcept override
    {
        return session_.load() != &session && inUse_.load() != &session;
    }

    /// Audio thread: records the block into the current session, if any.
    void process(const float* const* inputs, int numInputs, int numSamples) noexcept
    {
        RecordingSession* session = session_.load();
        for (;;)
        {
            inUse_.store(session);
            // stop() may have run between the load and the announcement; then the GUI thread may already
            // consider the session released, so it must not be touched.
            RecordingSession* const latest = session_.load();
            if (latest == session)
                break;
            session = latest;
        }
        if (session != nullptr)
            session->record(inputs, numInputs, numSamples);
        inUse_.store(nullptr);
    }

private:
    AtomicPointer session_{nullptr};
    AtomicPointer inUse_{nullptr};
};

using Recorder = BasicRecorder<>;

} // namespace snotes::engine
