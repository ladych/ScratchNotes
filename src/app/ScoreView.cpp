#include "app/ScoreView.h"

#include "app/Theme.h"
#include "i18n/Translate.h"
#include "io/Utf8Path.h"
#include "notation/MusicXml.h"
#include "notation/Score.h"
#include "ui/ScoreDisplay.h"

#include <algorithm>
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
constexpr int kCursorIntervalMs = 30;
constexpr float kCursorMargin = 3.0F;
constexpr float kCursorAlpha = 0.3F;

juce::File verovioData()
{
    return juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("verovio-data");
}

} // namespace

/// The drawn pages, one below the other, with the cursor.
class ScoreView::Pages final : public juce::Component
{
public:
    void show(const std::vector<std::unique_ptr<juce::XmlElement>>& svgs, int width)
    {
        drawables_.clear();
        tops_.clear();
        notes_.clear();
        cursor_.reset();
        float height = 0.0F;
        for (const auto& svg : svgs)
            if (svg != nullptr)
            {
                const auto pageHeight = svg->getStringAttribute("height")
                                            .upToFirstOccurrenceOf("px", false, false)
                                            .getFloatValue();
                drawables_.push_back(juce::Drawable::createFromSVG(*svg));
                tops_.push_back(height);
                if (drawables_.back() != nullptr)
                    collectNotes(*drawables_.back(), height);
                height += pageHeight * kZoom;
            }
        setSize(width, static_cast<int>(std::ceil(height)));
        repaint();
    }

    [[nodiscard]] bool empty() const { return drawables_.empty(); }

    /// Moves the cursor to a tick of the score; true if it moved.
    bool showCursor(std::optional<double> tick)
    {
        const auto area = tick ? ui::cursorArea(notes_, *tick) : std::nullopt;
        std::optional<juce::Rectangle<float>> cursor;
        if (area)
            cursor =
                juce::Rectangle<float>::leftTopRightBottom(area->left, area->top, area->right, area->bottom)
                    .expanded(kCursorMargin, 0.0F);
        if (cursor == cursor_)
            return false;
        cursor_ = cursor;
        repaint();
        return true;
    }

    [[nodiscard]] std::optional<juce::Rectangle<float>> cursor() const { return cursor_; }

    void paint(juce::Graphics& g) override
    {
        if (cursor_)
        {
            g.setColour(theme::colours().accent.withAlpha(kCursorAlpha));
            g.fillRoundedRectangle(*cursor_, kCursorMargin);
        }
        for (std::size_t page = 0; page < drawables_.size(); ++page)
            if (drawables_[page] != nullptr)
                drawables_[page]->draw(g, 1.0F, toPage(page));
    }

private:
    [[nodiscard]] juce::AffineTransform toPage(std::size_t page) const
    {
        return juce::AffineTransform::scale(kZoom).translated(0.0F, tops_[page]);
    }

    /// The notes of a page with their bars, by the ids the MusicXML gave them.
    void collectNotes(juce::Drawable& page, float top)
    {
        // Drawable::draw paints the page's own component content shifted to its drawable origin.
        const auto transform = juce::AffineTransform::translation(page.getPosition().toFloat())
                                   .followedBy(page.getTransform())
                                   .followedBy(juce::AffineTransform::scale(kZoom).translated(0.0F, top));
        // Each group of the SVG with the bar it is in.
        std::vector<std::pair<const juce::Component*, ui::Area>> groups{{&page, {}}};
        while (!groups.empty())
        {
            const auto [parent, bar] = groups.back();
            groups.pop_back();
            for (const auto* child : parent->getChildren())
            {
                const auto id = child->getComponentID();
                const auto area = areaOf(page, *child, transform);
                if (const auto span = notation::noteSpanOf(id.toStdString()))
                    notes_.push_back({.span = *span, .area = area, .bar = bar});
                groups.emplace_back(child, id.startsWith(notation::kBarIdPrefix) ? area : bar);
            }
        }
    }

    static ui::Area
    areaOf(const juce::Component& page, const juce::Component& child, const juce::AffineTransform& transform)
    {
        const auto bounds =
            page.getLocalArea(&child, child.getLocalBounds().toFloat()).transformedBy(transform);
        return {.left = bounds.getX(),
                .top = bounds.getY(),
                .right = bounds.getRight(),
                .bottom = bounds.getBottom()};
    }

    std::vector<std::unique_ptr<juce::Drawable>> drawables_;
    std::vector<float> tops_;
    std::vector<ui::DrawnNote> notes_;
    std::optional<juce::Rectangle<float>> cursor_;
};

ScoreView::ScoreView(juce::ValueTree project,
                     std::function<std::optional<std::filesystem::path>()> projectFolder,
                     std::function<double()> positionSeconds)
    : project_(std::move(project)), projectFolder_(std::move(projectFolder)),
      positionSeconds_(std::move(positionSeconds)),
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
    startTimer(kCursorIntervalMs);
}

ScoreView::~ScoreView()
{
    stopTimer();
    cancelPendingUpdate();
}

void ScoreView::timerCallback()
{
    const auto quarter = ui::scoreQuarterAt(project_, positionSeconds_());
    if (!pages_->showCursor(quarter ? std::optional{*quarter * notation::kTicksPerBeat} : std::nullopt))
        return;
    // Scrolls along when the cursor leaves the visible part.
    const auto area = pages_->cursor();
    const auto visible = viewport_.getViewArea().toFloat();
    if (area && !visible.contains(*area))
        viewport_.setViewPosition(0, std::max(0, static_cast<int>(area->getY()) - kPadding));
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
