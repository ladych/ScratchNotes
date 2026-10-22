#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace vrv
{
class Toolkit;
}

namespace snotes::notation
{

/// Draws a score with Verovio (F-NA-01): MusicXML in, one SVG per page out. Verovio needs its resource
/// directory (fonts, external/verovio/data), which the build copies next to the program as verovio-data/.
/// Pages have no header, footer or divider between systems. Not thread-safe; one renderer per thread.
class ScoreRenderer
{
public:
    explicit ScoreRenderer(const std::filesystem::path& resourceDirectory);
    ~ScoreRenderer();

    ScoreRenderer(const ScoreRenderer&) = delete;
    ScoreRenderer& operator=(const ScoreRenderer&) = delete;
    ScoreRenderer(ScoreRenderer&&) = delete;
    ScoreRenderer& operator=(ScoreRenderer&&) = delete;

    /// False if the resource directory has no fonts; then nothing can be drawn.
    [[nodiscard]] bool ready() const noexcept { return ready_; }
    /// Verovio's version, e.g. "6.3.0".
    [[nodiscard]] std::string version() const;

    /// Width of the pages in pixels of the SVG (Verovio allows 100 to 60000); it applies from the next load.
    /// Pages are as high as their systems need.
    void setPageWidth(int width);

    /// Reads a MusicXML document; returns the number of pages, or nothing if it cannot be read.
    [[nodiscard]] std::optional<int> load(const std::string& musicXml);
    /// SVG of a page of the loaded document, counted from 1; empty for a page that does not exist.
    [[nodiscard]] std::string page(int pageNumber);

private:
    std::unique_ptr<vrv::Toolkit> toolkit_;
    bool ready_ = false;
    int pages_ = 0;
};

} // namespace snotes::notation
