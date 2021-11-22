#pragma once

#include <JuceHeader.h>


enum Slope {
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// A data structure representing all of the parameter values

struct ChainSettings
{
    float peakFreq { 0 }, preakGainInDecibels { 0 }, peakQ {1.0f};
    float lowCutFreq { 0 }, highCutFreq { 0 };
    Slope lowCutSlope { Slope::Slope_12 }, highCutSlope { Slope::Slope_12 };
};

// Helper function that will give us all of these parameter values in our data struct.
// Extracting our paramaters from the audio processor value tree state
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

// Creating a filter alias
using Filter = juce::dsp::IIR::Filter<float>;

// Each IIR low pass or high pass filter has a responce of 12db per octave, so if we want to have a chain with a responce of 48db per octave we need to
// have a chain of 4 filters.
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

// The chain represents the whole mono signal path (lowpass - peak - highpass)
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

enum ChainPositions {
    LowCut,
    Peak,
    HighCut
};

using Coefficients = Filter::CoefficientsPtr;
void updateCoefficients(Coefficients& old, const Coefficients& replacements);

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);


template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType& chain, const CoefficientType& coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}


template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType& lowCut,
                     const CoefficientType& cutCoefficients,
                     const Slope& lowCutSlope)
{
    
    // First, bypass all filters in the chain
    lowCut.template setBypassed<0>(true);
    lowCut.template setBypassed<1>(true);
    lowCut.template setBypassed<2>(true);
    lowCut.template setBypassed<3>(true);
    
    // turn on filters based on selected slope
    
    switch( lowCutSlope )
    {
            
        case Slope_48:
        {
            update<3>(lowCut, cutCoefficients);
        }
        case Slope_36:
        {
            update<2>(lowCut, cutCoefficients);
        }
        case Slope_24:
        {
            update<1>(lowCut, cutCoefficients);
        }
        case Slope_12:
        {
            update<0>(lowCut, cutCoefficients);
        }
    }
}

inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                       sampleRate,
                                                                                       2 * (chainSettings.lowCutSlope + 1));
}

inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
                                                                                      sampleRate,
                                                                                      2 * (chainSettings.highCutSlope + 1));
}

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};

private:
    

    
    // We need 2 instances of the monochain to do stereo processing.
    MonoChain leftChain, rightChain;
    
    void updatePeakFilter(const ChainSettings& chainSettings);

    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);
    
    void updateFilters();
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
