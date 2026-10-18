#pragma once

#include "transcription/BasicPitch.h"
#include "transcription/NoteCreation.h"

#include <string>
#include <vector>

namespace snotes::transcription::test
{

/// A note of the Python reference (tools/export-basic-pitch-reference.py).
struct ReferenceNote
{
    FrameNote note;
    double startSeconds = 0.0;
    double endSeconds = 0.0;
};

/// Reads tests/data/transcription/<name>: little-endian float32 values.
[[nodiscard]] std::vector<float> readFloats(const std::string& name);
/// The activations Python's run_inference produced for <prefix>-22050.f32.
[[nodiscard]] Activations readActivations(const std::string& prefix);
[[nodiscard]] std::vector<ReferenceNote> readNotes(const std::string& prefix);
/// Path of the Basic Pitch model in resources/Models.
[[nodiscard]] std::string modelFile();

} // namespace snotes::transcription::test
