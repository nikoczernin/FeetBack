#include "Processor.h"
#include "Editor.h"

namespace {
juce::NormalisableRange<float> logarithmic(float low,float high) {
    return {low,high,
        [](float a,float b,float x){return a*std::pow(b/a,x);},
        [](float a,float b,float x){return std::log(x/a)/std::log(b/a);},
        [](float,float,float x){return std::round(x);}};
}
}
juce::AudioProcessorValueTreeState::ParameterLayout FeetBackProcessor::parameters() {
    using namespace juce;
    using Float=AudioParameterFloat;
    using Attr=AudioParameterFloatAttributes;
    AudioProcessorValueTreeState::ParameterLayout result;
    for(const auto* id: {"amount","sensitivity","mix"}) {
        const String name=id;
        const float initial=name=="amount" ? 65.0f : name=="sensitivity" ? 60.0f : 45.0f;
        result.add(std::make_unique<Float>(ParameterID{id,1},name.substring(0,1).toUpperCase()+name.substring(1),
            NormalisableRange<float>(0,100,1),initial,
            Attr().withStringFromValueFunction([](float x,int){return String(roundToInt(x))+" %";})));
    }
    for(const auto* id: {"attack","release"}) {
        const bool attack=String(id)=="attack";
        result.add(std::make_unique<Float>(ParameterID{id,1},attack ? "Attack" : "Release",
            logarithmic(10,attack ? 10000.0f : 15000.0f),attack ? 700.0f : 600.0f,
            Attr().withStringFromValueFunction([](float x,int){return String(roundToInt(x))+" ms";})
                .withValueFromStringFunction([](const String& s){return s.getFloatValue()*(s.containsIgnoreCase("s") && !s.containsIgnoreCase("ms") ? 1000.0f : 1.0f);})));
    }
    result.add(std::make_unique<Float>(ParameterID{"tone",1},"Tone",logarithmic(200,12000),6000,
        Attr().withStringFromValueFunction([](float x,int){return String(roundToInt(x))+" Hz";})
            .withValueFromStringFunction([](const String& s){return s.getFloatValue()*(s.containsIgnoreCase("k") ? 1000.0f : 1.0f);})));
    result.add(std::make_unique<AudioParameterChoice>(ParameterID{"harmonic",1},"Harmonic",
        StringArray{"Auto low","Auto high","Sub octave","Unison","Octave","Octave + fifth","Two octaves","Two octaves + third"},0));
    result.add(std::make_unique<AudioParameterChoice>(ParameterID{"operation",1},"Switch mode",
        StringArray{"Latching","Momentary"},0));
    result.add(std::make_unique<AudioParameterBool>(ParameterID{"engaged",1},"Feedback on",true));
    result.add(std::make_unique<AudioParameterBool>(ParameterID{"bypass",1},"Bypass",false));
    return result;
}
FeetBackProcessor::FeetBackProcessor()
    : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
        .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
      state(*this,nullptr,"FeetBackState",parameters()) {
    const std::array<const char*,9> ids{"amount","attack","release","sensitivity","tone","mix","harmonic","engaged","bypass"};
    for(size_t i=0;i<ids.size();++i) values[i]=state.getRawParameterValue(ids[i]);
    setLatencySamples(0);
}
feetback::Parameters FeetBackProcessor::read() const {
    feetback::Parameters p;
    p.amount=values[0]->load(); p.attackMs=values[1]->load(); p.releaseMs=values[2]->load();
    p.sensitivity=values[3]->load(); p.toneHz=values[4]->load(); p.mix=values[5]->load();
    p.harmonic=static_cast<int>(values[6]->load()); p.engaged=values[7]->load()>0.5f;
    p.bypass=values[8]->load()>0.5f;
    return p;
}
void FeetBackProcessor::prepareToPlay(double sr,int) {
    engine.setParameters(read()); engine.prepare(sr); setLatencySamples(0);
    envelope.store(0); pitch.store(0);
}
bool FeetBackProcessor::isBusesLayoutSupported(const BusesLayout& b) const {
    const auto in=b.getMainInputChannelSet();
    return in==b.getMainOutputChannelSet() && (in==juce::AudioChannelSet::mono() || in==juce::AudioChannelSet::stereo());
}
void FeetBackProcessor::process(juce::AudioBuffer<float>& buffer,bool hostBypass) {
    juce::ScopedNoDenormals noDenormals;
    auto p=read(); p.bypass=p.bypass || hostBypass;
    engine.setParameters(p);
    engine.process(buffer.getArrayOfWritePointers(),buffer.getNumChannels(),buffer.getNumSamples());
    envelope.store(engine.getEnvelope(),std::memory_order_relaxed);
    pitch.store(engine.getPitch(),std::memory_order_relaxed);
    harmonic.store(engine.getHarmonic(),std::memory_order_relaxed);
}
void FeetBackProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) { process(b,false); }
void FeetBackProcessor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&) { process(b,true); }
juce::AudioProcessorEditor* FeetBackProcessor::createEditor() { return new FeetBackEditor(*this); }
void FeetBackProcessor::getStateInformation(juce::MemoryBlock& bytes) {
    auto copy=state.copyState();
    // A physical momentary press must never remain held when reopening a session.
    if(state.getRawParameterValue("operation")->load()>0.5f)
        copy.getChildWithProperty("id","engaged").setProperty("value",0.0f,nullptr);
    if(auto xml=copy.createXml()) copyXmlToBinary(*xml,bytes);
}
void FeetBackProcessor::setStateInformation(const void* data,int bytes) {
    if(auto xml=getXmlFromBinary(data,bytes))
        if(xml->hasTagName(state.state.getType())) {
            auto restored=juce::ValueTree::fromXml(*xml);
            if(static_cast<float>(restored.getChildWithProperty("id","operation").getProperty("value",0.0f))>0.5f)
                restored.getChildWithProperty("id","engaged").setProperty("value",0.0f,nullptr);
            state.replaceState(restored);
        }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FeetBackProcessor(); }
