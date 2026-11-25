#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine.h"

class FeetBackProcessor final : public juce::AudioProcessor {
public:
    FeetBackProcessor();
    const juce::String getName() const override { return "FeetBack"; }
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    void reset() override { engine.reset(); }
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    using AudioProcessor::processBlockBypassed;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 15.3; }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::AudioProcessorParameter* getBypassParameter() const override { return state.getParameter("bypass"); }
    juce::AudioProcessorValueTreeState state;
    std::atomic<float> envelope{0},pitch{0},harmonic{1};
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout parameters();
    feetback::Parameters read() const;
    void process(juce::AudioBuffer<float>&,bool);
    feetback::Engine engine;
    std::array<std::atomic<float>*,9> values{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FeetBackProcessor)
};
