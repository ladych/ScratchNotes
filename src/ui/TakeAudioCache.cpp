#include "ui/TakeAudioCache.h"

#include "io/Utf8Path.h"

#include <algorithm>
#include <utility>

namespace snotes::ui
{

TakeAudioCache::TakeAudioCache(ITakeAudioLoader& loader, FolderSource folder)
    : loader_(loader), folder_(std::move(folder))
{
}

std::shared_ptr<const engine::TakeAudio> TakeAudioCache::audio(const std::string& relativeFile)
{
    const auto folder = folder_();
    if (!folder || relativeFile.empty())
        return nullptr;
    std::erase_if(entries_, [](const auto& entry) { return entry.second.expired(); });
    const auto file = *folder / io::pathFromUtf8(relativeFile);
    if (auto cached = entries_[file].lock())
        return cached;
    auto loaded = loader_.load(file);
    entries_[file] = loaded;
    return loaded;
}

std::size_t TakeAudioCache::size() const
{
    return static_cast<std::size_t>(std::count_if(
        entries_.begin(), entries_.end(), [](const auto& entry) { return !entry.second.expired(); }));
}

} // namespace snotes::ui
