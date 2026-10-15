#include "app/AboutDialog.h"

#include "i18n/Translate.h"

#include <cmath>

namespace snotes::app
{
namespace
{

constexpr int kWidth = 640;
constexpr int kHeight = 600;
constexpr int kMargin = 16;
constexpr int kLinkHeight = 22;
constexpr int kGap = 12;

juce::String utf8(const std::string& text)
{
    return juce::String::fromUTF8(text.c_str());
}

} // namespace

AboutDialog::AboutDialog(const ui::AboutInfo& info)
{
    addText(utf8(info.title), 22.0F, true);
    for (const auto& line : info.lines)
        addText(utf8(line), 15.0F, false);
    for (const auto& link : info.links)
        addLink(link);
    addGap();
    addText(i18n::trJuce("Third-party components"), 18.0F, true);
    for (const auto& component : info.components)
        addComponent(component);

    viewport_.setViewedComponent(&content_, false);
    viewport_.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport_);
    setSize(kWidth, kHeight);
}

void AboutDialog::resized()
{
    viewport_.setBounds(getLocalBounds());
    const int width = viewport_.getMaximumVisibleWidth() - 2 * kMargin;
    int y = kMargin;
    for (const auto& item : items_)
    {
        if (item == nullptr)
        {
            y += kGap;
            continue;
        }
        int height = kLinkHeight;
        if (auto* label = dynamic_cast<juce::Label*>(item.get()))
        {
            const auto font = label->getFont();
            juce::AttributedString text;
            text.append(label->getText(), font);
            juce::TextLayout layout;
            layout.createLayout(text, static_cast<float>(width));
            height = static_cast<int>(std::ceil(layout.getHeight())) + 6;
        }
        item->setBounds(kMargin, y, width, height);
        // A link is only clickable on its text.
        if (auto* link = dynamic_cast<juce::HyperlinkButton*>(item.get()))
            link->changeWidthToFitText();
        y += height;
    }
    content_.setSize(width + 2 * kMargin, y + kMargin);
}

void AboutDialog::addText(const juce::String& text, float fontHeight, bool bold)
{
    auto label = std::make_unique<juce::Label>(juce::String{}, text);
    label->setFont(juce::FontOptions{fontHeight, bold ? juce::Font::bold : juce::Font::plain});
    label->setJustificationType(juce::Justification::topLeft);
    label->setBorderSize({2, 0, 2, 0});
    // Wrap long lines instead of squeezing them.
    label->setMinimumHorizontalScale(1.0F);
    content_.addAndMakeVisible(*label);
    items_.push_back(std::move(label));
}

void AboutDialog::addLink(const ui::Link& link)
{
    auto button = std::make_unique<juce::HyperlinkButton>(utf8(link.text), juce::URL{utf8(link.url)});
    button->setFont(juce::FontOptions{15.0F}, false, juce::Justification::centredLeft);
    button->setTooltip(utf8(link.url));
    content_.addAndMakeVisible(*button);
    items_.push_back(std::move(button));
}

void AboutDialog::addComponent(const ui::ThirdPartyComponent& component)
{
    addGap();
    addText(utf8(component.name), 16.0F, true);
    addText(
        utf8(component.author + "\n" + component.usage + "\n" + i18n::tr("License: %1", component.license)),
        14.0F,
        false);
    if (!component.notice.empty())
        addText(utf8(component.notice), 14.0F, false);
    addLink({i18n::tr("Website: %1", component.website.text), component.website.url});
    addLink({i18n::tr("License text: %1", component.licenseText.text), component.licenseText.url});
}

void AboutDialog::addGap()
{
    items_.push_back(nullptr);
}

} // namespace snotes::app
