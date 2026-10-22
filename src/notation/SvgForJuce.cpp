#include "notation/SvgForJuce.h"

#include <map>
#include <memory>
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

void rewrite(juce::XmlElement& element,
             const juce::XmlElement& root,
             const Glyphs& glyphs,
             const juce::String& ink)
{
    if (element.hasTagName("svg") && element.hasAttribute("viewBox"))
        scaleViewBox(element, root);
    else if (element.hasTagName("use"))
        useGlyph(element, glyphs);
    colour(element, ink);
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
    return root;
}

} // namespace snotes::notation
