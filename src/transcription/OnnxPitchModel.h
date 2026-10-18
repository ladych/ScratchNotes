#pragma once

#include "transcription/PitchModel.h"

#include <cstddef>
#include <memory>

namespace snotes::transcription
{

/// Basic Pitch run by ONNX Runtime. The model is loaded from memory, so the program and the command line
/// tool can carry it built in (Pflichtenheft 3). Throws std::runtime_error if the bytes are no valid model.
class OnnxPitchModel : public IPitchModel
{
public:
    OnnxPitchModel(const void* modelData, std::size_t modelSize);
    ~OnnxPitchModel() override;
    OnnxPitchModel(const OnnxPitchModel&) = delete;
    OnnxPitchModel& operator=(const OnnxPitchModel&) = delete;
    OnnxPitchModel(OnnxPitchModel&&) = delete;
    OnnxPitchModel& operator=(OnnxPitchModel&&) = delete;

    void run(std::span<const float> window, std::span<float> frames, std::span<float> onsets) override;

private:
    struct Session;
    std::unique_ptr<Session> session_;
};

} // namespace snotes::transcription
