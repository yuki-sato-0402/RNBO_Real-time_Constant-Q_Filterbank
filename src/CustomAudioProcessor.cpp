#include "CustomAudioProcessor.h"
#include "CustomAudioEditor.h"

CustomAudioProcessor::CustomAudioProcessor() 
: AudioProcessor (BusesProperties()
                  #if ! JucePlugin_IsMidiEffect
                  #if ! JucePlugin_IsSynth
                  .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                  #endif
                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                  #endif
                  ),
parameters(*this, nullptr, juce::Identifier("PARAMETERS"),
    juce::AudioProcessorValueTreeState::ParameterLayout {
    std::make_unique<juce::AudioParameterFloat>(ParameterID { "Q_factor",  1}, "Q_factor",
    juce::NormalisableRange<float>(1.f, 2.f, 0.01f, 1.f), 1.f),
    std::make_unique<juce::AudioParameterFloat>(ParameterID { "noiseFloor",  2}, "noiseFloor",
    juce::NormalisableRange<float>(0.f, 30.f, 0.1f, 1.f), 5.f),
    std::make_unique<juce::AudioParameterFloat>(ParameterID { "overtoneThreshold",  3}, "overtoneThreshold",
    juce::NormalisableRange<float>(10.f, 80.f, 1.f, 1.f), 50.f),
    std::make_unique<juce::AudioParameterFloat>(ParameterID { "polyTolerance",  4}, "polyTolerance",
    juce::NormalisableRange<float>(10.f, 50.f, 1.f, 1.f), 20.f),
    }
  )
{
 
 
  for (RNBO::ParameterIndex i = 0; i < rnboObject.getNumParameters(); ++i){
    RNBO::ParameterInfo info;
    rnboObject.getParameterInfo (i, &info);

    if (info.visible){
      auto paramID = juce::String (rnboObject.getParameterId (i));
      std::cout << "paramID " << paramID << std::endl;
      std::cout << "Parameter Index: " << i << std::endl;
    //  std::cout << "Min Value: " << info.min << std::endl;
    //  std::cout << "Max Value: " << info.max << std::endl;
        
      // Each apvts parameter id and range must be the same as the rnbo param object's.
      // If you hit this assertion then you need to fix the incorrect id in ParamIDs.h.
      jassert (parameters.getParameter (paramID) != nullptr);

      constexpr float epsilon = 1e-5f;
      jassert (std::abs(static_cast<float>(info.min) - parameters.getParameterRange(paramID).start) < epsilon);
      jassert (std::abs(static_cast<float>(info.max) - parameters.getParameterRange(paramID).end) < epsilon);
      apvtsParamIdToRnboParamIndex[paramID] = i;
    

      parameters.addParameterListener(paramID, this);
      rnboObject.setParameterValue(i, parameters.getRawParameterValue(paramID)->load());  
      
    } 
  }
}

void CustomAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    rnboObject.prepareToProcess (sampleRate, static_cast<size_t> (samplesPerBlock));
    ringBuffer.resize(buffSize);

    setBufferData();
}
 
void CustomAudioProcessor::releaseResources()
{
}


void CustomAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{ 
    juce::ignoreUnused (midiMessages);
    rnboObject.process(
        buffer.getArrayOfWritePointers(), static_cast<RNBO::Index>(buffer.getNumChannels()),
        buffer.getArrayOfWritePointers(), static_cast<RNBO::Index>(buffer.getNumChannels()),
        static_cast<RNBO::Index> (buffer.getNumSamples())
    );     

    updateBuffer(RNBORingbuffer.get());
}

void CustomAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    std::cout << "Parameter Changed: " << parameterID << " New Value: " << newValue << std::endl;
    rnboObject.setParameterValue (apvtsParamIdToRnboParamIndex[parameterID], newValue);
}

juce::AudioProcessorEditor* CustomAudioProcessor::createEditor()
{
   return new CustomAudioEditor (*this,  parameters);
}

bool CustomAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}


const juce::String CustomAudioProcessor::getName() const{
    return "RNBO_CombGrain";
}

bool CustomAudioProcessor::acceptsMidi() const
{
    return false;
}

bool CustomAudioProcessor::producesMidi() const 
{
    return false;
}   
 
bool CustomAudioProcessor::isMidiEffect() const
{
    return false;
}

double CustomAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}
 
int CustomAudioProcessor::getNumPrograms()
{
    return 1;   
}
 
int CustomAudioProcessor::getCurrentProgram()
{
    return 0;
}
 
void CustomAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}
 
const juce::String CustomAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}
 
void CustomAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}


void CustomAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CustomAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

void CustomAudioProcessor::setBufferData()
{
    RNBORingbuffer = std::make_unique<float[]>(buffSize * 2);
   
    std::fill(RNBORingbuffer.get(), RNBORingbuffer.get() + buffSize, 0.0f);
   

    RNBO::Float32AudioBuffer bufferType(1, this->rnboObject.getSampleRate());

    this->rnboObject.setExternalData(
        "OutputBuff",
        reinterpret_cast<char*>(RNBORingbuffer.get()),
        buffSize * sizeof(float),
        bufferType
    );
}

void CustomAudioProcessor::updateBuffer(float* RNBORingbuffer)
{
    for (int i = 0; i < buffSize; ++i){
            ringBuffer[i] = RNBORingbuffer[i];
    }
}
