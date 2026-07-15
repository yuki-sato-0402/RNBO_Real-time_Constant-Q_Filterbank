#include "CustomAudioProcessor.h"
#include "CustomAudioEditor.h"

CustomAudioEditor::CustomAudioEditor (CustomAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
: AudioProcessorEditor (&p), valueTreeState(vts), audioProcessor(p), spectrogramDisplay (p.getRingBufferPointer())
{
    addAndMakeVisible(spectrogramDisplay);

     
    addAndMakeVisible(Q_factorSlider);
    Q_factorSliderAttachment.reset (new SliderAttachment (valueTreeState, "Q_factor", Q_factorSlider));
    Q_factorSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    Q_factorSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, Q_factorSlider.getTextBoxWidth(), Q_factorSlider.getTextBoxHeight());
    Q_factorSlider.setTextValueSuffix (" %");
    Q_factorSlider.setColour(juce::Slider::backgroundColourId, juce::Colours::white);
    Q_factorSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey.withAlpha(0.75f));
    Q_factorSlider.setColour(juce::Slider::thumbColourId , juce::Colours::darkgrey.brighter(1.5));
    Q_factorSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    Q_factorSlider.setColour(juce::Slider::textBoxOutlineColourId , juce::Colours::white); 

    addAndMakeVisible(Q_factorLabel);
    addAndMakeVisible(Q_factorLabel);
    Q_factorLabel.setText ("Q Factor", juce::dontSendNotification);
    Q_factorLabel.setJustificationType(juce::Justification::centred);
  
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
    int sliderHeight = area.getHeight() - spectrogramHeight - (padding * 2) - 20;
    Q_factorSlider.setBounds(padding, spectrogramDisplay.getBottom() + 15, (area.getWidth() - (padding * 2)) / 2, sliderHeight);
    Q_factorLabel.setBounds(Q_factorSlider.getX(), Q_factorSlider.getY() , Q_factorSlider.getWidth(), Q_factorSlider.getTextBoxHeight());
}
