#include "notation/ScoreRenderer.h"

#include "io/Utf8Path.h"

#include <toolkit.h>

namespace snotes::notation
{

ScoreRenderer::ScoreRenderer(const std::filesystem::path& resourceDirectory)
    : toolkit_(std::make_unique<vrv::Toolkit>(false))
{
    ready_ = toolkit_->SetResourcePath(io::utf8FromPath(resourceDirectory));
}

ScoreRenderer::~ScoreRenderer() = default;

std::string ScoreRenderer::version() const
{
    // Verovio appends the Git commit it was built from, "[undefined]" in our build.
    const auto full = toolkit_->GetVersion();
    return full.substr(0, full.find_first_not_of("0123456789."));
}

std::optional<int> ScoreRenderer::load(const std::string& musicXml)
{
    pages_ = 0;
    if (!ready_ || !toolkit_->LoadData(musicXml))
        return std::nullopt;
    pages_ = toolkit_->GetPageCount();
    return pages_;
}

std::string ScoreRenderer::page(int pageNumber)
{
    if (pageNumber < 1 || pageNumber > pages_)
        return {};
    return toolkit_->RenderToSVG(pageNumber);
}

} // namespace snotes::notation
