#pragma once
#include "Processor.h"

class FeetBackLook final : public juce::LookAndFeel_V4 {
public:
    FeetBackLook();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
};

class FeetBackEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit FeetBackEditor(FeetBackProcessor&);
    ~FeetBackEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    void setEngaged(bool);
    void describe(juce::Component*);
    void refreshStatus();
    FeetBackProcessor& audioProcessor;
    FeetBackLook look;
    std::unique_ptr<juce::Drawable> feet;
    std::array<juce::Slider,6> knobs;
    std::array<juce::Label,6> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,6> knobAttachments;
    juce::ComboBox harmonicChoice,operationChoice;
    juce::Label harmonicLabel,operationLabel,status,note;
    juce::TextButton feedback,bypass;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> harmonicAttachment,operationAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    juce::String help;
    bool held=false;
    float displayEnvelope=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FeetBackEditor)
};
