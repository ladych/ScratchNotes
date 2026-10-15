#include "engine/SnapshotExchange.h"

#include <gtest/gtest.h>

#include <atomic>
#include <functional>
#include <memory>
#include <numeric>
#include <thread>
#include <utility>
#include <vector>

namespace snotes::engine
{
namespace
{

struct Snapshot
{
    explicit Snapshot(int initialValue, int* destroyedCounter = nullptr)
        : value(initialValue), destroyed(destroyedCounter)
    {
    }
    ~Snapshot()
    {
        if (destroyed != nullptr)
            ++*destroyed;
    }
    Snapshot(const Snapshot&) = delete;
    Snapshot& operator=(const Snapshot&) = delete;
    Snapshot(Snapshot&&) = delete;
    Snapshot& operator=(Snapshot&&) = delete;

    int value;
    int* destroyed;
};

/// Runs a hook right after the next load, i.e. between the audio thread's read of the current
/// snapshot and its announcement in the hazard pointer.
std::function<void()> runAfterNextLoad;

class RacingAtomic
{
public:
    explicit RacingAtomic(const Snapshot* value) : value_(value) {}

    const Snapshot* load() const
    {
        const Snapshot* const value = value_;
        if (auto hook = std::exchange(runAfterNextLoad, nullptr))
            hook();
        return value;
    }

    void store(const Snapshot* value) { value_ = value; }

private:
    const Snapshot* value_;
};

TEST(SnapshotExchangeTest, Q04_HasNoSnapshotBeforeTheFirstPublish)
{
    SnapshotExchange<Snapshot> exchange;

    EXPECT_EQ(exchange.acquire(), nullptr);
}

TEST(SnapshotExchangeTest, Q04_AudioThreadSeesTheLatestPublishedSnapshot)
{
    SnapshotExchange<Snapshot> exchange;

    exchange.publish(std::make_unique<Snapshot>(1));
    EXPECT_EQ(exchange.acquire()->value, 1);

    exchange.publish(std::make_unique<Snapshot>(2));
    EXPECT_EQ(exchange.acquire()->value, 2);
}

TEST(SnapshotExchangeTest, Q04_KeepsTheSnapshotTheAudioThreadIsReading)
{
    int destroyed = 0;
    SnapshotExchange<Snapshot> exchange;
    exchange.publish(std::make_unique<Snapshot>(1, &destroyed));
    const auto* reading = exchange.acquire();

    exchange.publish(std::make_unique<Snapshot>(2, &destroyed));
    exchange.publish(std::make_unique<Snapshot>(3, &destroyed));

    EXPECT_EQ(reading->value, 1);
    EXPECT_EQ(destroyed, 1); // only snapshot 2: neither current nor in use
    EXPECT_EQ(exchange.numOwned(), 2U);
}

TEST(SnapshotExchangeTest, Q04_FreesTheOldSnapshotOnceTheAudioThreadMovedOn)
{
    int destroyed = 0;
    SnapshotExchange<Snapshot> exchange;
    exchange.publish(std::make_unique<Snapshot>(1, &destroyed));
    static_cast<void>(exchange.acquire());
    exchange.publish(std::make_unique<Snapshot>(2, &destroyed));

    static_cast<void>(exchange.acquire());
    exchange.collectGarbage();

    EXPECT_EQ(destroyed, 1);
    EXPECT_EQ(exchange.numOwned(), 1U);
}

TEST(SnapshotExchangeTest, Q04_RetriesWhenASnapshotIsReplacedWhileBeingAcquired)
{
    int destroyed = 0;
    SnapshotExchange<Snapshot, RacingAtomic> exchange;
    exchange.publish(std::make_unique<Snapshot>(1, &destroyed));

    // The GUI thread publishes snapshot 2 right after the audio thread read snapshot 1 but before
    // it marked snapshot 1 as in use, so snapshot 1 is freed and must not be returned.
    runAfterNextLoad = [&] { exchange.publish(std::make_unique<Snapshot>(2, &destroyed)); };
    const auto* acquired = exchange.acquire();

    EXPECT_EQ(destroyed, 1);
    EXPECT_EQ(acquired->value, 2);
}

TEST(SnapshotExchangeTest, Q04_AudioThreadReadsConsistentSnapshotsWhileGuiPublishes)
{
    // Run under ThreadSanitizer (E-09): reading while publishing must be free of data races.
    struct Notes
    {
        std::vector<int> values;
        int sum = 0;
    };
    constexpr int kPublishes = 2000;
    SnapshotExchange<Notes> exchange;
    exchange.publish(std::make_unique<Notes>());
    std::atomic<bool> done{false};
    std::atomic<int> inconsistent{0};

    std::thread audio{
        [&]
        {
            while (!done.load())
            {
                const auto* notes = exchange.acquire();
                if (std::accumulate(notes->values.begin(), notes->values.end(), 0) != notes->sum)
                    ++inconsistent;
            }
        }};

    for (int i = 1; i <= kPublishes; ++i)
    {
        auto notes = std::make_unique<Notes>();
        notes->values.assign(static_cast<std::size_t>(i % 64), i);
        notes->sum = (i % 64) * i;
        exchange.publish(std::move(notes));
    }
    done.store(true);
    audio.join();

    EXPECT_EQ(inconsistent.load(), 0);
    EXPECT_LE(exchange.numOwned(), 2U);
}

} // namespace
} // namespace snotes::engine
