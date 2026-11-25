#include "Processor.h"
#include <iostream>
#include <stdexcept>
namespace {
void check(bool value,const char* text) { if(!value) throw std::runtime_error(text); }
void set(FeetBackProcessor& p,const char* id,float value) {
    auto* parameter=p.state.getParameter(id); parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
}
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    try {
        FeetBackProcessor p;
        for(int channels:{1,2}) {
            auto layout=p.getBusesLayout();
            layout.getChannelSet(true,0)=layout.getChannelSet(false,0)=juce::AudioChannelSet::canonicalChannelSet(channels);
            check(p.setBusesLayout(layout),"mono/stereo layouts"); p.prepareToPlay(48000,64);
            check(p.getLatencySamples()==0,"zero reported latency");
            juce::AudioBuffer<float> audio(channels,257); juce::MidiBuffer midi;
            for(int c=0;c<channels;++c) for(int i=0;i<257;++i) audio.setSample(c,i,std::sin(static_cast<float>(i)*0.1f)*0.2f);
            const juce::AudioBuffer<float> original(audio);
            set(p,"bypass",1); p.processBlock(audio,midi);
            for(int c=0;c<channels;++c) check(std::memcmp(audio.getReadPointer(c),original.getReadPointer(c),257*sizeof(float))==0,"plugin bypass");
            set(p,"bypass",0); p.processBlockBypassed(audio,midi);
            for(int c=0;c<channels;++c) check(std::memcmp(audio.getReadPointer(c),original.getReadPointer(c),257*sizeof(float))==0,"host bypass");
        }
        set(p,"amount",73); set(p,"attack",1234); set(p,"release",987); set(p,"tone",4321); set(p,"harmonic",5); set(p,"mix",29);
        juce::MemoryBlock bytes; p.getStateInformation(bytes); FeetBackProcessor restored;
        restored.setStateInformation(bytes.getData(),static_cast<int>(bytes.getSize()));
        for(const auto* id:{"amount","attack","release","tone","harmonic","mix"})
            check(std::abs(p.state.getRawParameterValue(id)->load()-restored.state.getRawParameterValue(id)->load())<0.001f,"session recall");
        set(p,"operation",1); set(p,"engaged",1); p.getStateInformation(bytes);
        restored.setStateInformation(bytes.getData(),static_cast<int>(bytes.getSize()));
        check(restored.state.getRawParameterValue("engaged")->load()<0.5f,"momentary press cannot stick across session restore");
        FeetBackProcessor ui;
        std::unique_ptr<juce::AudioProcessorEditor> editor(ui.createEditor());
        for(auto* child:editor->getChildren()) if(auto* slider=dynamic_cast<juce::Slider*>(child)) {
            slider->setValue(slider->getMaximum(),juce::sendNotificationSync);
            const auto now=juce::Time::getCurrentTime();
            juce::MouseEvent event(juce::Desktop::getInstance().getMainMouseSource(),{40,40},{},1,0,0,0,0,slider,slider,now,{40,40},now,2,false);
            slider->mouseDoubleClick(event);
            check(std::abs(slider->getValue()-slider->getDoubleClickReturnValue())<0.01,"double-click reset");
        }
        auto* button=dynamic_cast<juce::TextButton*>(editor->findChildWithID("feedback"));
        auto* operation=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("operation"));
        check(button && operation,"footswitch controls present");
        button->onClick(); check(ui.state.getRawParameterValue("engaged")->load()<0.5f,"latching off");
        button->onClick(); check(ui.state.getRawParameterValue("engaged")->load()>0.5f,"latching on");
        operation->setSelectedId(2,juce::sendNotificationSync);
        check(ui.state.getRawParameterValue("engaged")->load()<0.5f,"entering momentary disarms feedback");
        button->setState(juce::Button::buttonDown);
        check(ui.state.getRawParameterValue("engaged")->load()>0.5f,"momentary held");
        button->setState(juce::Button::buttonNormal);
        check(ui.state.getRawParameterValue("engaged")->load()<0.5f,"momentary released");
        operation->setSelectedId(1,juce::sendNotificationSync); set(ui,"engaged",1);
        if(argc==2) {
            auto stream=juce::File(argv[1]).createOutputStream();
            check(stream && stream->setPosition(0) && stream->truncate().wasOk(),"preview output");
            juce::PNGImageFormat png; check(png.writeImageToStream(editor->createComponentSnapshot(editor->getLocalBounds(),true,1.5f),*stream),"preview render");
        }
        button->setState(juce::Button::buttonNormal);
        std::cout<<"PASS: mono/stereo, zero latency, exact bypass, session recall, knob reset, latching/momentary footswitch.\n";
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
