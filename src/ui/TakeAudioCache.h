#pragma once

#include "engine/TakeAudio.h"

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace snotes::ui
{

/// Reads a take file into memory (F-WI-02); null if it cannot be read.
class ITakeAudioLoader
{
public:
    virtual ~ITakeAudioLoader() = default;
    [[nodiscard]] virtual std::shared_ptr<const engine::TakeAudio>
    load(const std::filesystem::path& file) = 0;

protected:
    ITakeAudioLoader() = default;
    ITakeAudioLoader(const ITakeAudioLoader&) = default;
    ITakeAudioLoader(ITakeAudioLoader&&) = default;
    ITakeAudioLoader& operator=(const ITakeAudioLoader&) = default;
    ITakeAudioLoader& operator=(ITakeAudioLoader&&) = default;
};

/// The take audio of the snapshots (model::TakeAudioSource): every file is read once and shared for as long
/// as a snapshot plays it. The cache itself only keeps weak references, so a take that no snapshot plays any
/// more is freed with the last snapshot, on the GUI thread.
class TakeAudioCache
{
public:
    using FolderSource = std::function<std::optional<std::filesystem::path>()>;

    TakeAudioCache(ITakeAudioLoader& loader, FolderSource folder);

    /// Audio of a file relative to the project folder; null without folder or if it cannot be read.
    [[nodiscard]] std::shared_ptr<const engine::TakeAudio> audio(const std::string& relativeFile);
    /// Files still held by a snapshot; for tests and diagnostics.
    [[nodiscard]] std::size_t size() const;

private:
    ITakeAudioLoader& loader_;
    FolderSource folder_;
    std::map<std::filesystem::path, std::weak_ptr<const engine::TakeAudio>> entries_;
};

} // namespace snotes::ui
