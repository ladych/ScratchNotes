#include "transcription/OnnxRuntime.h"

#include <onnxruntime_c_api.h>

namespace snotes::transcription
{

std::string onnxRuntimeVersion()
{
    return OrtGetApiBase()->GetVersionString();
}

} // namespace snotes::transcription
