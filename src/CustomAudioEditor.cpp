#include "CustomAudioProcessor.h"
#include "CustomAudioEditor.h"

CustomAudioEditor::CustomAudioEditor (CustomAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
: AudioProcessorEditor (&p), valueTreeState(vts), audioProcessor(p), spectrogramDisplay (p.getRingBufferPointer())
{
    addAndMakeVisible(spectrogramDisplay);

     
    addAndMakeVisible(Q_factorSlider);
    Q_factorSliderAttachment.reset (new SliderAttachment (valueTreeState, "Q_factor", Q_factorSlider));
    Q_factorSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    Q_factorSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, Q_factorSlider.getTextBoxWidth(), Q_factorSlider.getTextBoxHeight());
    Q_factorSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white);
    Q_factorSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::darkgrey.withAlpha(0.75f));
    Q_factorSlider.setColour(juce::Slider::thumbColourId , juce::Colours::darkgrey.brighter(1.5));
    Q_factorSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    Q_factorSlider.setColour(juce::Slider::textBoxOutlineColourId , juce::Colours::white); 

    addAndMakeVisible(Q_factorLabel);
    Q_factorLabel.setText ("Q_factor", juce::dontSendNotification);
    Q_factorLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(overtoneThresholdSlider);
    overtoneThresholdSliderAttachment.reset (new SliderAttachment (valueTreeState, "overtoneThreshold", overtoneThresholdSlider));
    overtoneThresholdSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    overtoneThresholdSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, overtoneThresholdSlider.getTextBoxWidth(), overtoneThresholdSlider.getTextBoxHeight());
    overtoneThresholdSlider.setTextValueSuffix (" %");
    overtoneThresholdSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white);
    overtoneThresholdSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::darkgrey.withAlpha(0.75f));
    overtoneThresholdSlider.setColour(juce::Slider::thumbColourId , juce::Colours::darkgrey.brighter(1.5));
    overtoneThresholdSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);

    addAndMakeVisible(overtoneThresholdLabel);
    overtoneThresholdLabel.setText ("overtoneThreshold", juce::dontSendNotification);
    overtoneThresholdLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(polyToleranceSlider);
    polyToleranceSliderAttachment.reset (new SliderAttachment (valueTreeState, "polyTolerance", polyToleranceSlider));
    polyToleranceSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    polyToleranceSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, polyToleranceSlider.getTextBoxWidth(), polyToleranceSlider.getTextBoxHeight());
    polyToleranceSlider.setTextValueSuffix (" %");
    polyToleranceSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white);
    polyToleranceSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::darkgrey.withAlpha(0.75f));
    polyToleranceSlider.setColour(juce::Slider::thumbColourId , juce::Colours::darkgrey.brighter(1.5));
    polyToleranceSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
  
    addAndMakeVisible(polyToleranceLabel);
    polyToleranceLabel.setText ("polyTolerance", juce::dontSendNotification);
    polyToleranceLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(noiseFloorSlider);
    noiseFloorSliderAttachment.reset (new SliderAttachment (valueTreeState, "noiseFloor", noiseFloorSlider));
    noiseFloorSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    noiseFloorSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, noiseFloorSlider.getTextBoxWidth(), noiseFloorSlider.getTextBoxHeight());
    noiseFloorSlider.setTextValueSuffix (" %");
    noiseFloorSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white);
    noiseFloorSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::darkgrey.withAlpha(0.75f));
    noiseFloorSlider.setColour(juce::Slider::thumbColourId , juce::Colours::darkgrey.brighter(1.5));
    noiseFloorSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);

    addAndMakeVisible(noiseFloorLabel);
    noiseFloorLabel.setText ("noiseFloor", juce::dontSendNotification);
    noiseFloorLabel.setJustificationType(juce::Justification::centred); 


    setSize(800, 500); // Make the plugin wider so the 88 keyboard is more usable
}


void CustomAudioEditor::paint (Graphics& g)
{
    g.fillAll(juce::Colour(8, 10, 15));
}

void CustomAudioEditor::resized()
{
    auto area = getLocalBounds();
    const int padding = 20;

    // Give the spectrogram display 65% of the total height
    int spectrogramHeight = static_cast<int>(area.getHeight() * 0.65f);
    spectrogramDisplay.setBounds(padding, padding, area.getWidth() - (padding * 2), spectrogramHeight);

    // Position the Q Factor dial below the spectrogram
    int sliderHeight = area.getHeight() - spectrogramHeight - (padding * 2) - 30;
    int sliderWidth = (area.getWidth() - (padding * 5)) / 4; // 4 sliders with padding between them
    Q_factorSlider.setBounds(padding, spectrogramDisplay.getBottom() + 25, sliderWidth, sliderHeight);
    overtoneThresholdSlider.setBounds(Q_factorSlider.getRight() + padding, spectrogramDisplay.getBottom() + 25, sliderWidth, sliderHeight);
    polyToleranceSlider.setBounds(overtoneThresholdSlider.getRight() + padding, spectrogramDisplay.getBottom() + 25, sliderWidth, sliderHeight);
    noiseFloorSlider.setBounds(polyToleranceSlider.getRight() + padding, spectrogramDisplay.getBottom() + 25, sliderWidth, sliderHeight);

    Q_factorLabel.setBounds(Q_factorSlider.getX(), Q_factorSlider.getY()- 20 , Q_factorSlider.getWidth(), Q_factorSlider.getTextBoxHeight());
    overtoneThresholdLabel.setBounds(overtoneThresholdSlider.getX(), overtoneThresholdSlider.getY() - 20, overtoneThresholdSlider.getWidth(), overtoneThresholdSlider.getTextBoxHeight());
    polyToleranceLabel.setBounds(polyToleranceSlider.getX(), polyToleranceSlider.getY() - 20, polyToleranceSlider.getWidth(), polyToleranceSlider.getTextBoxHeight());
    noiseFloorLabel.setBounds(noiseFloorSlider.getX(), noiseFloorSlider.getY() - 20, noiseFloorSlider.getWidth(), noiseFloorSlider.getTextBoxHeight());
}
