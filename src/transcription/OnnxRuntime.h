#pragma once

#include <string>

namespace snotes::transcription
{

/// Version of the ONNX Runtime library the program loaded, e.g. "1.31.0". The note detection (F-TX-02) runs
/// the Basic Pitch model with it; AP0 only checks that the library is built in and loads on every platform.
[[nodiscard]] std::string onnxRuntimeVersion();

} // namespace snotes::transcription
