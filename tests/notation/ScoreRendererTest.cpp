#include "notation/ScoreRenderer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

namespace snotes::notation
{
namespace
{

using ::testing::HasSubstr;
using ::testing::Not;

/// One bar of E-guitar in standard tuning as notes and tablature: open low E, then fret 2 on the A string.
const std::string kGuitarBar = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE score-partwise PUBLIC "-//Recordare//DTD MusicXML 4.0 Partwise//EN" "http://www.musicxml.org/dtds/partwise.dtd">
<score-partwise version="4.0">
  <part-list><score-part id="P1"><part-name>Gitarre</part-name></score-part></part-list>
  <part id="P1">
    <measure number="1">
      <attributes>
        <divisions>1</divisions>
        <time><beats>4</beats><beat-type>4</beat-type></time>
        <staves>2</staves>
        <clef number="1"><sign>G</sign><line>2</line><clef-octave-change>-1</clef-octave-change></clef>
        <clef number="2"><sign>TAB</sign><line>5</line></clef>
        <staff-details number="2"><staff-lines>6</staff-lines>
          <staff-tuning line="1"><tuning-step>E</tuning-step><tuning-octave>2</tuning-octave></staff-tuning>
          <staff-tuning line="2"><tuning-step>A</tuning-step><tuning-octave>2</tuning-octave></staff-tuning>
          <staff-tuning line="3"><tuning-step>D</tuning-step><tuning-octave>3</tuning-octave></staff-tuning>
          <staff-tuning line="4"><tuning-step>G</tuning-step><tuning-octave>3</tuning-octave></staff-tuning>
          <staff-tuning line="5"><tuning-step>B</tuning-step><tuning-octave>3</tuning-octave></staff-tuning>
          <staff-tuning line="6"><tuning-step>E</tuning-step><tuning-octave>4</tuning-octave></staff-tuning>
        </staff-details>
      </attributes>
      <note><pitch><step>E</step><octave>3</octave></pitch><duration>2</duration><type>half</type><staff>1</staff></note>
      <note><pitch><step>B</step><octave>3</octave></pitch><duration>2</duration><type>half</type><staff>1</staff></note>
      <backup><duration>4</duration></backup>
      <note><pitch><step>E</step><octave>2</octave></pitch><duration>2</duration><type>half</type><staff>2</staff>
        <notations><technical><string>6</string><fret>0</fret></technical></notations></note>
      <note><pitch><step>B</step><octave>2</octave></pitch><duration>2</duration><type>half</type><staff>2</staff>
        <notations><technical><string>5</string><fret>2</fret></technical></notations></note>
    </measure>
  </part>
</score-partwise>)";

TEST(ScoreRendererTest, FNA01_TheBuiltInVerovioIsTheIntegratedRelease)
{
    const ScoreRenderer renderer{SNOTES_VEROVIO_DATA_DIR};
    EXPECT_TRUE(renderer.ready());
    EXPECT_EQ(renderer.version(), "6.3.0");
}

TEST(ScoreRendererTest, FNA01_DrawsNotesAndTablatureFromMusicXmlAsSvg)
{
    ScoreRenderer renderer{SNOTES_VEROVIO_DATA_DIR};

    ASSERT_EQ(renderer.load(kGuitarBar), 1);
    const auto svg = renderer.page(1);

    EXPECT_THAT(svg, HasSubstr("<svg"));
    EXPECT_THAT(svg, HasSubstr("class=\"tabGrp"));
}

TEST(ScoreRendererTest, FNA01_PagesOutsideTheDocumentAreEmpty)
{
    ScoreRenderer renderer{SNOTES_VEROVIO_DATA_DIR};
    ASSERT_EQ(renderer.load(kGuitarBar), 1);

    EXPECT_EQ(renderer.page(0), "");
    EXPECT_EQ(renderer.page(2), "");
}

TEST(ScoreRendererTest, FNA01_TextThatIsNoScoreIsNotLoaded)
{
    ScoreRenderer renderer{SNOTES_VEROVIO_DATA_DIR};

    EXPECT_EQ(renderer.load("keine Noten"), std::nullopt);
    EXPECT_EQ(renderer.page(1), "");
}

TEST(ScoreRendererTest, FNA01_PagesHaveTheChosenWidthAndNoFooter)
{
    ScoreRenderer renderer{SNOTES_VEROVIO_DATA_DIR};

    renderer.setPageWidth(1200);
    ASSERT_EQ(renderer.load(kGuitarBar), 1);
    const auto svg = renderer.page(1);

    EXPECT_THAT(svg, HasSubstr("width=\"1200px\""));
    EXPECT_THAT(svg, Not(HasSubstr("pgFoot")));
}

TEST(ScoreRendererTest, FNA01_WithoutFontsNothingIsDrawn)
{
    ScoreRenderer renderer{"/no/such/verovio-data"};

    EXPECT_FALSE(renderer.ready());
    EXPECT_EQ(renderer.load(kGuitarBar), std::nullopt);
}

} // namespace
} // namespace snotes::notation
