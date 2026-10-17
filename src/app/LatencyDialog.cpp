#include "app/LatencyDialog.h"

#include "i18n/Translate.h"

namespace snotes::app
{
namespace
{

constexpr int kWidth = 520;
constexpr int kHeight = 330;
constexpr int kPadding = 12;
constexpr int kLineHeight = 26;
constexpr int kLabelWidth = 150;
constexpr int kPollMs = 50;
constexpr double kMaxOffsetMs = 50.0;

juce::String fromUtf8(const std::string& text)
{
    return juce::String::fromUTF8(text.c_str());
}

} // namespace

LatencyDialog::LatencyDialog(ui::LatencyCalibration& calibration,
                             ui::LatencyCompensation& compensation,
                             const std::vector<std::string>& inputNames)
    : calibration_(calibration), compensation_(compensation)
{
    help_.setText(
        i18n::trJuce("Connect an output of the interface to an input with a cable, choose that input "
                     "and press \"Measure\". The measurement plays short clicks: turn speakers and "
                     "headphones down."),
        juce::dontSendNotification);
    help_.setJustificationType(juce::Justification::topLeft);
    inputLabel_.setText(i18n::trJuce("Loopback input"), juce::dontSendNotification);
    for (std::size_t index = 0; index < inputNames.size(); ++index)
        input_.addItem(fromUtf8(std::to_string(index + 1) + " · " + inputNames[index]),
                       static_cast<int>(index) + 1);
    input_.setSelectedItemIndex(0, juce::dontSendNotification);
    measure_.setButtonText(i18n::trJuce("Measure"));
    measure_.onClick = [this] { measure(); };
    result_.setJustificationType(juce::Justification::topLeft);
    offsetLabel_.setText(i18n::trJuce("Offset (ms)"), juce::dontSendNotification);
    offset_.setSliderStyle(juce::Slider::IncDecButtons);
    offset_.setRange(-kMaxOffsetMs, kMaxOffsetMs, 0.1);
    offset_.setValue(compensation_.offsetMs(), juce::dontSendNotification);
    offset_.onValueChange = [this]
    {
        static_cast<void>(compensation_.setOffsetMs(offset_.getValue()));
        showCompensation();
    };
    for (auto* component : std::initializer_list<juce::Component*>{
             &help_, &inputLabel_, &input_, &measure_, &result_, &offsetLabel_, &offset_, &compensationText_})
        addAndMakeVisible(component);
    showCompensation();
    setSize(kWidth, kHeight);
}

LatencyDialog::~LatencyDialog()
{
    stopTimer();
    calibration_.cancel();
}

void LatencyDialog::resized()
{
    auto area = getLocalBounds().reduced(kPadding);
    help_.setBounds(area.removeFromTop(3 * kLineHeight));
    area.removeFromTop(kPadding);
    auto line = area.removeFromTop(kLineHeight);
    inputLabel_.setBounds(line.removeFromLeft(kLabelWidth));
    measure_.setBounds(line.removeFromRight(kLabelWidth - kPadding));
    line.removeFromRight(kPadding);
    input_.setBounds(line);
    area.removeFromTop(kPadding);
    result_.setBounds(area.removeFromTop(2 * kLineHeight));
    area.removeFromTop(kPadding);
    line = area.removeFromTop(kLineHeight);
    offsetLabel_.setBounds(line.removeFromLeft(kLabelWidth));
    offset_.setBounds(line.removeFromLeft(kLabelWidth));
    area.removeFromTop(kPadding);
    compensationText_.setBounds(area.removeFromTop(2 * kLineHeight));
}

void LatencyDialog::timerCallback()
{
    const auto text = calibration_.poll();
    if (calibration_.isRunning())
        return;
    stopTimer();
    measure_.setEnabled(true);
    if (text)
        result_.setText(fromUtf8(*text), juce::dontSendNotification);
    offset_.setValue(compensation_.offsetMs(), juce::dontSendNotification);
    showCompensation();
}

void LatencyDialog::measure()
{
    calibration_.start(std::max(0, input_.getSelectedItemIndex()));
    measure_.setEnabled(false);
    result_.setText(i18n::trJuce("Measuring..."), juce::dontSendNotification);
    startTimer(kPollMs);
}

void LatencyDialog::showCompensation()
{
    compensationText_.setText(fromUtf8(calibration_.compensationText()), juce::dontSendNotification);
}

} // namespace snotes::app
