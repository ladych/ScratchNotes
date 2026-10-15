#pragma once

#include <algorithm>
#include <atomic>
#include <memory>
#include <utility>
#include <vector>

namespace snotes::engine
{

/// Hands immutable snapshots from the GUI thread to the audio thread (Pflichtenheft chapter 4).
///
/// The audio thread calls acquire() once per block: two atomic stores and loads, no allocation, no
/// lock, no free (Q-04). The GUI thread owns every snapshot and frees old ones in publish() or
/// collectGarbage(), never one the audio thread may still read. For that the audio thread announces
/// the snapshot it reads in inUse_ (a single hazard pointer) and re-checks that it is still current,
/// so a snapshot replaced in between is never dereferenced.
///
/// AtomicPointer is std::atomic<const T*> in production; tests inject a type that simulates a
/// publish racing with acquire() (E-07).
template <typename T, typename AtomicPointer = std::atomic<const T*>> class SnapshotExchange
{
public:
    SnapshotExchange() = default;
    ~SnapshotExchange() = default;

    SnapshotExchange(const SnapshotExchange&) = delete;
    SnapshotExchange& operator=(const SnapshotExchange&) = delete;
    SnapshotExchange(SnapshotExchange&&) = delete;
    SnapshotExchange& operator=(SnapshotExchange&&) = delete;

    /// GUI thread: makes the snapshot current and frees snapshots no longer in use.
    void publish(std::unique_ptr<const T> snapshot)
    {
        const T* const published = snapshot.get();
        owned_.push_back(std::move(snapshot));
        current_.store(published);
        collectGarbage();
    }

    /// GUI thread: frees every snapshot that is neither current nor read by the audio thread.
    void collectGarbage()
    {
        const T* const current = current_.load();
        const T* const inUse = inUse_.load();
        std::erase_if(owned_,
                      [current, inUse](const auto& snapshot)
                      { return snapshot.get() != current && snapshot.get() != inUse; });
    }

    /// Audio thread: the current snapshot, valid until the next acquire(); nullptr before the first
    /// publish().
    const T* acquire() noexcept
    {
        const T* snapshot = current_.load();
        for (;;)
        {
            inUse_.store(snapshot);
            const T* const latest = current_.load();
            if (latest == snapshot)
                return snapshot;
            snapshot = latest;
        }
    }

    /// Number of snapshots still allocated; for tests and diagnostics.
    [[nodiscard]] std::size_t numOwned() const { return owned_.size(); }

private:
    AtomicPointer current_{nullptr};
    AtomicPointer inUse_{nullptr};
    std::vector<std::unique_ptr<const T>> owned_;
};

} // namespace snotes::engine
