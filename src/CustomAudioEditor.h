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
private:
    juce::AudioProcessorValueTreeState& valueTreeState; // ✅ Hold by reference
    CustomAudioProcessor& audioProcessor;
    
    juce::Slider Q_factorSlider;
    juce::Slider overtoneThresholdSlider;
    juce::Slider polyToleranceSlider;
    juce::Slider noiseFloorSlider;


    juce::Label Q_factorLabel;
    juce::Label overtoneThresholdLabel;
    juce::Label polyToleranceLabel;
    juce::Label noiseFloorLabel;



    std::unique_ptr<SliderAttachment> Q_factorSliderAttachment;
    std::unique_ptr<SliderAttachment> overtoneThresholdSliderAttachment;
    std::unique_ptr<SliderAttachment> polyToleranceSliderAttachment;
    std::unique_ptr<SliderAttachment> noiseFloorSliderAttachment;

    SpectrogramDisplay spectrogramDisplay;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomAudioEditor)
};
