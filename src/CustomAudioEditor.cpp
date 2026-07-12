#include "CustomAudioProcessor.h"
#include "CustomAudioEditor.h"

CustomAudioEditor::CustomAudioEditor (CustomAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
: AudioProcessorEditor (&p), valueTreeState(vts), audioProcessor(p), spectrogramDisplay (p.getRingBufferPointer())
{
    addAndMakeVisible(spectrogramDisplay);

     
    addAndMakeVisible(Q_factorDial);
    Q_factorDialAttachment.reset (new SliderAttachment (valueTreeState, "Q_factor", Q_factorDial));
    Q_factorDial.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    Q_factorDial.setTextBoxStyle (juce::Slider::TextBoxBelow, false, Q_factorDial.getTextBoxWidth(), Q_factorDial.getTextBoxHeight());
    Q_factorDial.setTextValueSuffix (" %");
    Q_factorDial.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(45, 48, 58));
    Q_factorDial.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(255, 110, 0));
    Q_factorDial.setColour(juce::Slider::thumbColourId , juce::Colour(255, 160, 40));
    Q_factorDial.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    Q_factorDial.setColour(juce::Slider::textBoxOutlineColourId , juce::Colour(45, 48, 58));
    
    addAndMakeVisible(Q_factorLabel);
    addAndMakeVisible(Q_factorLabel);
    Q_factorLabel.setText ("Q Factor", juce::dontSendNotification);
    Q_factorLabel.setJustificationType(juce::Justification::centred);
    Q_factorLabel.setColour(juce::Label::textColourId, juce::Colour(180, 185, 195));

    setSize(800, 500); // Make the plugin wider so the 88 keyboard is more usable
}


void CustomAudioEditor::paint (Graphics& g)
{
    g.fillAll(juce::Colour(15, 18, 26)); 
}

void CustomAudioEditor::resized()
{
    auto area = getLocalBounds();
    const int padding = 20;

    // Give the spectrogram display 65% of the total height
    int spectrogramHeight = static_cast<int>(area.getHeight() * 0.65f);
    spectrogramDisplay.setBounds(padding, padding, area.getWidth() - (padding * 2), spectrogramHeight);

    // Position the Q Factor dial below the spectrogram
    int dialSize = 90;
    Q_factorDial.setBounds(padding, spectrogramDisplay.getBottom() + 15, dialSize, dialSize);
    Q_factorLabel.setBounds(Q_factorDial.getX(), Q_factorDial.getBottom() + 2, Q_factorDial.getWidth(), 20);
}
