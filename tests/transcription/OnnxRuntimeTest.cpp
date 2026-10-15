#include "transcription/OnnxRuntime.h"

#include <gtest/gtest.h>

namespace snotes::transcription
{
namespace
{

TEST(OnnxRuntimeTest, FTX02_ThePinnedOnnxRuntimeLoads)
{
    EXPECT_EQ(onnxRuntimeVersion(), "1.31.0");
}

} // namespace
} // namespace snotes::transcription
