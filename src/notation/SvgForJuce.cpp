#include "notation/SvgForJuce.h"

#include <array>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace snotes::notation
{
namespace
{

using Glyphs = std::map<juce::String, juce::XmlElement>;

/// The shapes in defs by the id of their group, as Verovio writes its font glyphs.
Glyphs glyphsOf(const juce::XmlElement& root)
{
    Glyphs glyphs;
    for (const auto* defs : root.getChildWithTagNameIterator("defs"))
        for (const auto* glyph : defs->getChildIterator())
            if (const auto* shape = glyph->getFirstChildElement())
                glyphs.emplace(glyph->getStringAttribute("id"), *shape);
    return glyphs;
}

double pixels(const juce::String& length)
{
    return length.upToFirstOccurrenceOf("px", false, false).getDoubleValue();
}

/// The inner svg of Verovio maps its viewBox onto the size of the outer one.
void scaleViewBox(juce::XmlElement& svg, const juce::XmlElement& root)
{
    const auto box = juce::StringArray::fromTokens(svg.getStringAttribute("viewBox"), " ", "");
    const auto scaleX = pixels(root.getStringAttribute("width")) / box[2].getDoubleValue();
    const auto scaleY = pixels(root.getStringAttribute("height")) / box[3].getDoubleValue();
    svg.setTagName("g");
    // JUCE draws no text in a font family it cannot find, such as Verovio's "Times, serif".
    svg.removeAllAttributes();
    svg.setAttribute("transform", "scale(" + juce::String{scaleX} + ", " + juce::String{scaleY} + ")");
}

void useGlyph(juce::XmlElement& use, const Glyphs& glyphs)
{
    const auto found =
        glyphs.find(use.getStringAttribute("xlink:href").fromFirstOccurrenceOf("#", false, false));
    use.setTagName("g");
    use.removeAttribute("xlink:href");
    if (found != glyphs.end())
        use.addChildElement(std::make_unique<juce::XmlElement>(found->second).release());
}

void colour(juce::XmlElement& element, const juce::String& ink)
{
    const bool shape = element.hasTagName("path") || element.hasTagName("rect") ||
                       element.hasTagName("polygon") || element.hasTagName("polyline") ||
                       element.hasTagName("ellipse");
    if (shape && !element.hasAttribute("stroke"))
        element.setAttribute("stroke", ink);
    if ((shape || element.hasTagName("text")) && !element.hasAttribute("fill"))
        element.setAttribute("fill", ink);
    // Verovio sizes its text in nested tspans; JUCE would take the 0 of the outer one.
    if (element.getStringAttribute("font-size") == "0px")
        element.removeAttribute("font-size");
}

/// SMuFL metronome notes and the text characters that stand in for them.
constexpr std::array<std::pair<juce::juce_wchar, juce::juce_wchar>, 2> kMetronomeNotes{
    {{0xECA5, 0x2669}, {0xECA7, 0x266A}}};
/// The note of the text font is smaller than that of the music font at the same size.
constexpr double kNoteToTextSize = 0.8;

/// Verovio writes the note of a tempo mark in its music font, which JUCE cannot find, so the mark showed only
/// "= 92": a quarter or eighth note of the text font takes its place.
void metronomeNote(juce::XmlElement& tspan)
{
    const auto text = tspan.getAllSubText();
    for (const auto& [music, standIn] : kMetronomeNotes)
    {
        if (text != juce::String::charToString(music))
            continue;
        tspan.deleteAllChildElements();
        tspan.addTextElement(juce::String::charToString(standIn));
        tspan.removeAttribute("font-family");
        tspan.setAttribute("font-size",
                           juce::String{pixels(tspan.getStringAttribute("font-size")) * kNoteToTextSize} +
                               "px");
    }
}

void rewrite(juce::XmlElement& element,
             const juce::XmlElement& root,
             const Glyphs& glyphs,
             const juce::String& ink)
{
    if (element.hasTagName("svg") && element.hasAttribute("viewBox"))
        scaleViewBox(element, root);
    else if (element.hasTagName("use"))
        useGlyph(element, glyphs);
    else if (element.hasTagName("tspan") && element.hasAttribute("font-family"))
        metronomeNote(element);
    colour(element, ink);
}

/// Where a group puts what it holds: point * scale + offset.
struct Placement
{
    double scaleX = 1.0;
    double scaleY = 1.0;
    double offsetX = 0.0;
    double offsetY = 0.0;
};

/// The placement inside an element with this transform; Verovio only translates and scales.
Placement within(Placement placement, const juce::String& transform)
{
    auto tokens = juce::StringArray::fromTokens(transform.replaceCharacters("(),", "   "), " ", "");
    tokens.removeEmptyStrings();
    for (int index = 0; index + 2 < tokens.size(); index += 3)
    {
        const auto first = tokens[index + 1].getDoubleValue();
        const auto second = tokens[index + 2].getDoubleValue();
        if (tokens[index] == "translate")
        {
            placement.offsetX += placement.scaleX * first;
            placement.offsetY += placement.scaleY * second;
        }
        else
        {
            placement.scaleX *= first;
            placement.scaleY *= second;
        }
    }
    return placement;
}

void place(juce::XmlElement& text, const Placement& placement)
{
    text.setAttribute("x", (placement.scaleX * text.getDoubleAttribute("x")) + placement.offsetX);
    text.setAttribute("y", (placement.scaleY * text.getDoubleAttribute("y")) + placement.offsetY);
    std::vector<juce::XmlElement*> open{&text};
    while (!open.empty())
    {
        auto* element = open.back();
        open.pop_back();
        if (element->hasAttribute("font-size"))
            element->setAttribute(
                "font-size",
                juce::String{pixels(element->getStringAttribute("font-size")) * placement.scaleY} + "px");
        for (auto* child : element->getChildIterator())
            open.push_back(child);
    }
}

/// JUCE gives text in a transformed group a transform of its own and then loses it in the bounds of the
/// group, so that it draws only some of the fret numbers. Text is therefore moved to the top level, with
/// its position and size already transformed.
void liftTexts(juce::XmlElement& root)
{
    std::vector<std::tuple<juce::XmlElement*, juce::XmlElement*, Placement>> texts;
    std::vector<std::pair<juce::XmlElement*, Placement>> open{{&root, Placement{}}};
    while (!open.empty())
    {
        const auto [element, placement] = open.back();
        open.pop_back();
        for (auto* child : element->getChildIterator())
        {
            if (child->hasTagName("text"))
                texts.emplace_back(child, element, placement);
            else
                open.emplace_back(child, within(placement, child->getStringAttribute("transform")));
        }
    }
    for (const auto& [text, parent, placement] : texts)
    {
        parent->removeChildElement(text, false);
        place(*text, placement);
        root.addChildElement(text);
    }
}

} // namespace

std::unique_ptr<juce::XmlElement> svgForJuce(const std::string& svg, const std::string& ink)
{
    auto root = juce::parseXML(juce::String::fromUTF8(svg.c_str()));
    if (root == nullptr || !root->hasTagName("svg"))
        return nullptr;
    const auto glyphs = glyphsOf(*root);
    root->deleteAllChildElementsWithTagName("defs");
    root->deleteAllChildElementsWithTagName("style");
    const auto colour = juce::String::fromUTF8(ink.c_str());
    std::vector<juce::XmlElement*> open{root.get()};
    while (!open.empty())
    {
        auto* element = open.back();
        open.pop_back();
        for (auto* child : element->getChildIterator())
        {
            rewrite(*child, *root, glyphs, colour);
            open.push_back(child);
        }
    }
    liftTexts(*root);
    return root;
}

} // namespace snotes::notation
