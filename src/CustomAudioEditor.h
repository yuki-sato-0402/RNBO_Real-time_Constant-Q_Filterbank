#include "JuceHeader.h"
#include "RNBO.h"
#include "SpectrogramDisplay.h"

class CustomAudioEditor : public juce::AudioProcessorEditor
{
public:
    CustomAudioEditor(CustomAudioProcessor& p, juce::AudioProcessorValueTreeState& vts);
    ~CustomAudioEditor() override = default;
    void paint (Graphics& g) override;
    void resized() override; 
    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

private:
    juce::AudioProcessorValueTreeState& valueTreeState; // ✅ Hold by reference
    CustomAudioProcessor& audioProcessor;
    
    juce::Slider Q_factorDial;


    juce::Label Q_factorLabel;


    std::unique_ptr<SliderAttachment> Q_factorDialAttachment;

    SpectrogramDisplay spectrogramDisplay;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomAudioEditor)
};
