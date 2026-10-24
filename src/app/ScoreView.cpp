#include "app/ScoreView.h"

#include "app/Theme.h"
#include "i18n/Translate.h"
#include "io/Utf8Path.h"
#include "ui/ScoreDisplay.h"

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace snotes::app
{
namespace
{

/// Verovio's pages are drawn at this size: its staff is then about as high as a track row's text.
constexpr float kZoom = 0.4F;
constexpr int kPadding = 8;

juce::File verovioData()
{
    return juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("verovio-data");
}

} // namespace

/// The drawn pages, one below the other.
class ScoreView::Pages final : public juce::Component
{
public:
    void show(const std::vector<std::unique_ptr<juce::XmlElement>>& svgs, int width)
    {
        drawables_.clear();
        heights_.clear();
        float height = 0.0F;
        for (const auto& svg : svgs)
            if (svg != nullptr)
            {
                const auto pageHeight = svg->getStringAttribute("height")
                                            .upToFirstOccurrenceOf("px", false, false)
                                            .getFloatValue();
                drawables_.push_back(juce::Drawable::createFromSVG(*svg));
                heights_.push_back(pageHeight * kZoom);
                height += pageHeight * kZoom;
            }
        setSize(width, static_cast<int>(std::ceil(height)));
        repaint();
    }

    [[nodiscard]] bool empty() const { return drawables_.empty(); }

    void paint(juce::Graphics& g) override
    {
        float top = 0.0F;
        for (std::size_t page = 0; page < drawables_.size(); ++page)
        {
            if (drawables_[page] != nullptr)
                drawables_[page]->draw(g, 1.0F, juce::AffineTransform::scale(kZoom).translated(0.0F, top));
            top += heights_[page];
        }
    }

private:
    std::vector<std::unique_ptr<juce::Drawable>> drawables_;
    std::vector<float> heights_;
};

ScoreView::ScoreView(juce::ValueTree project,
                     std::function<std::optional<std::filesystem::path>()> projectFolder)
    : project_(std::move(project)), projectFolder_(std::move(projectFolder)),
      renderer_(io::pathFromUtf8(verovioData().getFullPathName().toStdString())),
      pages_(std::make_unique<Pages>()), projectListener_(project_, [this] { triggerAsyncUpdate(); })
{
    viewport_.setViewedComponent(pages_.get(), false);
    viewport_.setScrollBarsShown(true, false);
    empty_.setText(i18n::trJuce("No notes yet. They appear when the note detection is done."),
                   juce::dontSendNotification);
    empty_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(viewport_);
    addChildComponent(empty_);
}

ScoreView::~ScoreView()
{
    cancelPendingUpdate();
}

void ScoreView::paint(juce::Graphics& g)
{
    g.fillAll(theme::colours().panel);
}

void ScoreView::resized()
{
    viewport_.setBounds(getLocalBounds().reduced(kPadding, 0));
    empty_.setBounds(getLocalBounds());
    triggerAsyncUpdate();
}

void ScoreView::lookAndFeelChanged()
{
    shown_.clear();
    triggerAsyncUpdate();
}

void ScoreView::handleAsyncUpdate()
{
    const auto path = ui::scorePath(project_, projectFolder_());
    const auto file =
        path ? juce::File{juce::String::fromUTF8(io::utf8FromPath(*path).c_str())} : juce::File{};
    const auto width = viewport_.getMaximumVisibleWidth();
    const auto key = file.getFullPathName() + "|" +
                     juce::String{file.getLastModificationTime().toMilliseconds()} + "|" +
                     juce::String{file.getSize()} + "|" + juce::String{width};
    if (key == shown_ || width <= 0)
        return;
    shown_ = key;
    showScore(file);
}

void ScoreView::showScore(const juce::File& file)
{
    const auto width = viewport_.getMaximumVisibleWidth();
    std::vector<std::unique_ptr<juce::XmlElement>> svgs;
    if (file.existsAsFile())
        svgs = ui::scorePages(renderer_,
                              file.loadFileAsString().toStdString(),
                              static_cast<int>(std::lround(static_cast<float>(width) / kZoom)),
                              "#" + theme::colours().text.toDisplayString(false).toStdString());
    pages_->show(svgs, width);
    empty_.setVisible(pages_->empty());
    viewport_.setVisible(!pages_->empty());
}

} // namespace snotes::app
