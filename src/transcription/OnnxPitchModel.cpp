#include "transcription/OnnxPitchModel.h"

#include "transcription/BasicPitch.h"

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <array>
#include <stdexcept>

namespace snotes::transcription
{
namespace
{

// Names in the exported model; Basic Pitch's inference.py maps them the same way.
constexpr const char* kInput = "serving_default_input_2:0";
constexpr const char* kFramesOutput = "StatefulPartitionedCall:1";
constexpr const char* kOnsetsOutput = "StatefulPartitionedCall:2";

} // namespace

struct OnnxPitchModel::Session
{
    Session(const void* modelData, std::size_t modelSize)
        : environment(ORT_LOGGING_LEVEL_ERROR, "ScratchNotes"),
          session(environment, modelData, modelSize, options())
    {
    }

    static Ort::SessionOptions options()
    {
        Ort::SessionOptions options;
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        return options;
    }

    Ort::Env environment;
    Ort::Session session;
    Ort::MemoryInfo memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
};

OnnxPitchModel::OnnxPitchModel(const void* modelData, std::size_t modelSize)
{
    try
    {
        session_ = std::make_unique<Session>(modelData, modelSize);
    }
    catch (...)
    {
        throw std::runtime_error("The note detection model could not be loaded");
    }
}

OnnxPitchModel::~OnnxPitchModel() = default;

void OnnxPitchModel::run(std::span<const float> window, std::span<float> frames, std::span<float> onsets)
{
    const std::array<std::int64_t, 3> shape{1, basicpitch::kWindowSamples, 1};
    // ONNX Runtime does not write to the input, it only takes a non-const pointer.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    auto* const samples = const_cast<float*>(window.data());
    auto input =
        Ort::Value::CreateTensor<float>(session_->memory, samples, window.size(), shape.data(), shape.size());
    const std::array<const char*, 1> inputNames{kInput};
    const std::array<const char*, 2> outputNames{kFramesOutput, kOnsetsOutput};
    auto outputs = session_->session.Run(
        Ort::RunOptions{nullptr}, inputNames.data(), &input, 1, outputNames.data(), outputNames.size());
    const auto copy = [](Ort::Value& value, std::span<float> target)
    {
        const auto* data = value.GetTensorData<float>();
        std::copy_n(data, target.size(), target.begin());
    };
    copy(outputs[0], frames);
    copy(outputs[1], onsets);
}

} // namespace snotes::transcription
