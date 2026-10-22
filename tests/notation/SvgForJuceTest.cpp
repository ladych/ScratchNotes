#include "notation/SvgForJuce.h"

#include "notation/ScoreRenderer.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace snotes::notation
{
namespace
{

/// The shape of Verovio's SVG: a glyph in defs, CSS, an inner svg with a viewBox ten times the page.
const std::string kVerovioSvg =
    R"svg(<svg width="2100px" height="2970px" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
  <defs>
    <g id="E0A4-abc"><path transform="scale(1,-1)" d="M0 0 L10 10 L20 0 Z"/></g>
    <g id="empty-abc"/>
  </defs>
  <style type="text/css">path {stroke:currentColor}</style>
  <svg class="definition-scale" color="black" font-family="Times, serif" viewBox="0 0 21000 29700">
    <g class="staff">
      <path d="M0 0 L100 0" stroke-width="13"/>
      <path d="M0 0 L100 0" stroke="red" fill="none"/>
      <polygon points="0,0 1,1 1,0"/>
      <use xlink:href="#E0A4-abc" transform="translate(100, 200) scale(0.72, 0.72)"/>
      <use xlink:href="#missing-abc" transform="translate(1, 2)"/>
      <text x="10" y="20" font-size="0px"><tspan font-size="324px">5</tspan></text>
      <text x="10" y="20" fill="blue">7</text>
      <rect x="0" y="0" width="5" height="5"/>
      <polyline points="0,0 1,1"/>
      <ellipse cx="1" cy="1" rx="1" ry="1"/>
      <svg width="10" height="10"><circle cx="1" cy="1" r="1"/></svg>
    </g>
  </svg>
</svg>)svg";

TEST(SvgForJuceTest, FNA01_TextThatIsNoSvgGivesNothing)
{
    EXPECT_EQ(svgForJuce("keine Noten", "#000000"), nullptr);
    EXPECT_EQ(svgForJuce("<html/>", "#000000"), nullptr);
}

TEST(SvgForJuceTest, FNA01_GlyphsAreCopiedToWhereTheyAreUsed)
{
    const auto svg = svgForJuce(kVerovioSvg, "#112233");
    ASSERT_NE(svg, nullptr);

    EXPECT_EQ(svg->getChildByName("defs"), nullptr);
    EXPECT_EQ(svg->getChildByName("style"), nullptr);
    const auto* staff = svg->getChildByName("g")->getChildByName("g");
    const auto* glyph = staff->getChildElement(3);
    EXPECT_EQ(glyph->getTagName(), "g");
    EXPECT_FALSE(glyph->hasAttribute("xlink:href"));
    EXPECT_EQ(glyph->getStringAttribute("transform"), "translate(100, 200) scale(0.72, 0.72)");
    const auto* shape = glyph->getChildByName("path");
    ASSERT_NE(shape, nullptr);
    EXPECT_EQ(shape->getStringAttribute("transform"), "scale(1,-1)");
    EXPECT_EQ(shape->getStringAttribute("fill"), "#112233");
    // A glyph Verovio did not define stays empty.
    EXPECT_EQ(staff->getChildElement(4)->getNumChildElements(), 0);
}

TEST(SvgForJuceTest, FNA01_TheInnerSvgBecomesAScaledGroup)
{
    const auto svg = svgForJuce(kVerovioSvg, "#112233");
    ASSERT_NE(svg, nullptr);

    const auto* group = svg->getChildByName("g");
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(svg->getChildByName("svg"), nullptr);
    EXPECT_EQ(group->getStringAttribute("transform"), "scale(0.1, 0.1)");
    EXPECT_FALSE(group->hasAttribute("viewBox"));
    EXPECT_FALSE(group->hasAttribute("font-family"));
}

TEST(SvgForJuceTest, FNA01_LinesShapesAndTextGetTheInk)
{
    const auto svg = svgForJuce(kVerovioSvg, "#112233");
    ASSERT_NE(svg, nullptr);

    const auto* staff = svg->getChildByName("g")->getChildByName("g");
    EXPECT_EQ(staff->getChildElement(0)->getStringAttribute("stroke"), "#112233");
    EXPECT_EQ(staff->getChildElement(0)->getStringAttribute("fill"), "#112233");
    EXPECT_EQ(staff->getChildElement(1)->getStringAttribute("stroke"), "red");
    EXPECT_EQ(staff->getChildElement(1)->getStringAttribute("fill"), "none");
    EXPECT_EQ(staff->getChildElement(2)->getStringAttribute("fill"), "#112233");
    const auto* fret = staff->getChildElement(5);
    EXPECT_EQ(fret->getStringAttribute("fill"), "#112233");
    EXPECT_FALSE(fret->hasAttribute("font-size"));
    EXPECT_EQ(fret->getChildByName("tspan")->getStringAttribute("font-size"), "324px");
    EXPECT_EQ(staff->getChildElement(6)->getStringAttribute("fill"), "blue");
    for (int index = 7; index < 10; ++index)
        EXPECT_EQ(staff->getChildElement(index)->getStringAttribute("stroke"), "#112233");
    // Only the inner svg with Verovio's viewBox is scaled; other elements keep their colours.
    const auto* nested = staff->getChildElement(10);
    EXPECT_EQ(nested->getTagName(), "svg");
    EXPECT_FALSE(nested->getChildElement(0)->hasAttribute("fill"));
}

/// Elements of the tree that JUCE cannot draw.
int undrawable(const juce::XmlElement& svg)
{
    int count = 0;
    std::vector<const juce::XmlElement*> open{&svg};
    while (!open.empty())
    {
        const auto* element = open.back();
        open.pop_back();
        for (const auto* child : element->getChildIterator())
        {
            count +=
                child->hasTagName("use") || child->hasTagName("svg") || child->hasTagName("style") ? 1 : 0;
            open.push_back(child);
        }
    }
    return count;
}

TEST(SvgForJuceTest, FNA01_APageOfVerovioHasNothingLeftThatJuceCannotDraw)
{
    ScoreRenderer renderer{SNOTES_VEROVIO_DATA_DIR};
    renderer.setPageWidth(1000);
    ASSERT_EQ(renderer.load(R"(<?xml version="1.0" encoding="UTF-8"?>
<score-partwise version="4.0">
  <part-list><score-part id="P1"><part-name>Bass</part-name></score-part></part-list>
  <part id="P1"><measure number="1">
    <attributes><divisions>1</divisions><time><beats>4</beats><beat-type>4</beat-type></time>
      <clef><sign>F</sign><line>4</line></clef></attributes>
    <note><pitch><step>E</step><octave>2</octave></pitch><duration>4</duration><type>whole</type></note>
  </measure></part>
</score-partwise>)"),
              1);
    const auto page = renderer.page(1);
    ASSERT_NE(page.find("<use"), std::string::npos);

    const auto svg = svgForJuce(page, "#000000");

    ASSERT_NE(svg, nullptr);
    EXPECT_EQ(svg->getStringAttribute("width"), "1000px");
    EXPECT_EQ(undrawable(*svg), 0);
}

} // namespace
} // namespace snotes::notation
