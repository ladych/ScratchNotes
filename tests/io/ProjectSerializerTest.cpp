#include "io/ProjectSerializer.h"

#include "model/ModelIds.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace snotes::io
{
namespace
{

namespace ids = model::ids;
using ::testing::HasSubstr;
using ::testing::Not;

class ProjectSerializerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto voice = project.addVoice(
            {.id = "v1", .name = "Gitarre 1", .profileId = "guitar7-gs", .inputDi = 0, .inputWet = 1});
        voice.setGain(0.75);
        voice.addTake({.id = "t1",
                       .diFile = "audio/t1-di.wav",
                       .wetFile = "audio/t1-wet.wav",
                       .startSample = 960,
                       .latencyOffsetSamples = 211,
                       .recordedAt = "2026-10-17T09:40:00"});
    }

    juce::ValueTree tree = model::ProjectFactory::createDefault("Notizen 2026-10-17 09-40");
    model::Project project{tree, nullptr};
};

TEST_F(ProjectSerializerTest, FPJ02_RoundTripKeepsTheWholeProject)
{
    const auto parsed = ProjectSerializer::fromXml(ProjectSerializer::toXml(tree));

    ASSERT_EQ(parsed.error, ProjectFileError::none);
    EXPECT_TRUE(parsed.project.isEquivalentTo(tree));
}

TEST_F(ProjectSerializerTest, FPJ02_FileHasTheFormatVersion)
{
    EXPECT_THAT(ProjectSerializer::toXml(tree), HasSubstr("formatVersion=\"1\""));
    EXPECT_FALSE(tree.hasProperty(ids::formatVersion));
}

TEST_F(ProjectSerializerTest, FPJ02_UmlautsSurvive)
{
    project.setName("Übung für Bäße");

    const auto parsed = ProjectSerializer::fromXml(ProjectSerializer::toXml(tree));

    EXPECT_EQ(model::Project(parsed.project, nullptr).name(), "Übung für Bäße");
}

TEST_F(ProjectSerializerTest, FPJ03_MissingAudioMarkerIsNotSaved)
{
    auto take = project.voice(0).take(0).tree();
    take.setProperty(ids::audioMissing, true, nullptr);

    EXPECT_THAT(ProjectSerializer::toXml(tree), Not(HasSubstr("audioMissing")));
    EXPECT_TRUE(take.hasProperty(ids::audioMissing));
}

TEST_F(ProjectSerializerTest, FPJ02_TextThatIsNoProjectIsRejected)
{
    EXPECT_EQ(ProjectSerializer::fromXml("not xml").error, ProjectFileError::notAProject);
    EXPECT_EQ(ProjectSerializer::fromXml("<KIT formatVersion=\"1\"/>").error, ProjectFileError::notAProject);
    EXPECT_EQ(ProjectSerializer::fromXml("<PROJECT name=\"x\"/>").error, ProjectFileError::notAProject);
}

TEST_F(ProjectSerializerTest, FPJ02_NewerFormatVersionIsRejected)
{
    EXPECT_EQ(ProjectSerializer::fromXml("<PROJECT formatVersion=\"2\"/>").error,
              ProjectFileError::newerFormatVersion);
}

TEST_F(ProjectSerializerTest, FPJ02_MissingSectionsAreAdded)
{
    const auto parsed = ProjectSerializer::fromXml("<PROJECT formatVersion=\"1\" name=\"alt\"/>");

    ASSERT_EQ(parsed.error, ProjectFileError::none);
    EXPECT_TRUE(parsed.project.getChildWithName(ids::voices).isValid());
    EXPECT_TRUE(parsed.project.getChildWithName(ids::mix).isValid());
    EXPECT_FALSE(parsed.project.hasProperty(ids::formatVersion));
}

} // namespace
} // namespace snotes::io
