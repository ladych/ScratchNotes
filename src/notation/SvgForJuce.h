#pragma once

#include <juce_core/juce_core.h>

#include <memory>
#include <string>

namespace snotes::notation
{

/// Verovio's SVG rewritten for juce::Drawable::createFromSVG, which reads no CSS, ignores the viewBox of a
/// nested svg and draws no glyphs used from defs: the glyphs are copied to where they are used, the inner
/// svg becomes a scaled group, and lines, glyphs and text get the colour ink ("#rrggbb") as attributes.
/// nullptr if the text is no SVG.
[[nodiscard]] std::unique_ptr<juce::XmlElement> svgForJuce(const std::string& svg, const std::string& ink);

} // namespace snotes::notation
