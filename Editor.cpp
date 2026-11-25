#include "Editor.h"
#include "BinaryData.h"

namespace {
const juce::Colour background(0xff10231e),panel(0xff1a3029),ink(0xfff3f0dc),muted(0xffa5b6a6);
const juce::Colour lime(0xffd4efa0),peach(0xfff0b18f);
const std::array<const char*,6> ids{"amount","attack","release","sensitivity","tone","mix"};
const std::array<const char*,6> names{"AMOUNT","ATTACK","RELEASE","SENSITIVITY","TONE","MIX"};
const std::array<const char*,6> descriptions{
    "Amount sets the strength of the generated feedback. Double-click any knob to reset it.",
    "Attack sets the feedback swell time after a note locks or feedback is engaged: 10 ms to 10 s.",
    "Release sets the fade back to dry guitar when feedback stops: 10 ms to 15 s.",
    "Higher sensitivity follows quieter notes and tolerates more pitch variation and noise.",
    "Tone softens or brightens the generated feedback. It does not filter your dry guitar.",
    "Mix sets the blend at full feedback: 0% dry, 100% feedback. The guitar stays dry before onset."};
juce::Font font(float size,bool bold=false) { return juce::Font(juce::FontOptions(size,bold ? juce::Font::bold : juce::Font::plain)); }
}
FeetBackLook::FeetBackLook() {
    setColour(juce::Slider::textBoxTextColourId,ink);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::TextButton::textColourOffId,ink);
    setColour(juce::TextButton::textColourOnId,background);
    setColour(juce::ComboBox::backgroundColourId,panel);
    setColour(juce::ComboBox::textColourId,ink);
    setColour(juce::ComboBox::outlineColourId,muted.withAlpha(0.25f));
    setColour(juce::ComboBox::arrowColourId,lime);
    setColour(juce::PopupMenu::backgroundColourId,panel);
    setColour(juce::PopupMenu::textColourId,ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,lime);
    setColour(juce::PopupMenu::highlightedTextColourId,background);
    setColour(juce::TextEditor::highlightColourId,lime.withAlpha(0.25f));
}
void FeetBackLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float value,float start,float end,juce::Slider& s) {
    auto bounds=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(w),static_cast<float>(h)).reduced(9);
    const float radius=std::min(bounds.getWidth(),bounds.getHeight())*0.5f;
    const auto centre=bounds.getCentre();
    const float angle=start+value*(end-start);
    const auto accent=s.getComponentID()=="release" || s.getComponentID()=="tone" ? peach : lime;
    juce::Path track,arc;
    track.addCentredArc(centre.x,centre.y,radius,radius,0,start,end,true);
    arc.addCentredArc(centre.x,centre.y,radius,radius,0,start,angle,true);
    g.setColour(muted.withAlpha(0.17f)); g.strokePath(track,juce::PathStrokeType(3));
    g.setColour(accent); g.strokePath(arc,juce::PathStrokeType(3,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    const auto body=juce::Rectangle<float>(radius*1.6f,radius*1.6f).withCentre(centre);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3c5144),body.getTopLeft(),juce::Colour(0xff22382f),body.getBottomRight(),false));
    g.fillEllipse(body);
    g.setColour(background); g.drawEllipse(body,1);
    g.setColour(ink); g.drawLine({centre.getPointOnCircumference(radius*0.34f,angle),centre.getPointOnCircumference(radius*0.65f,angle)},2.5f);
}
void FeetBackLook::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down) {
    auto colour=b.getToggleState() || down ? (b.getComponentID()=="bypass" ? peach : lime) : panel;
    if(over) colour=colour.brighter(0.08f);
    g.setColour(colour); g.fillRoundedRectangle(b.getLocalBounds().toFloat(),8);
    g.setColour(muted.withAlpha(0.25f)); g.drawRoundedRectangle(b.getLocalBounds().toFloat().reduced(0.5f),8,1);
}
FeetBackEditor::FeetBackEditor(FeetBackProcessor& p):AudioProcessorEditor(p),audioProcessor(p) {
    setLookAndFeel(&look);
    feet=juce::Drawable::createFromImageData(FeetBackArt::feet_svg,FeetBackArt::feet_svgSize);
    for(size_t i=0;i<knobs.size();++i) {
        auto& k=knobs[i]; k.setName(names[i]); k.setComponentID(ids[i]); k.setDescription(descriptions[i]);
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
        k.setTextBoxStyle(juce::Slider::TextBoxBelow,false,126,24);
        addAndMakeVisible(k);
        labels[i].setText(names[i],juce::dontSendNotification); labels[i].setFont(font(11.5f,true));
        labels[i].setColour(juce::Label::textColourId,muted); labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);
        knobAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],k);
        auto* parameter=p.state.getParameter(ids[i]); k.setDoubleClickReturnValue(true,parameter->convertFrom0to1(parameter->getDefaultValue()));
    }
    harmonicChoice.setName("Harmonic"); harmonicChoice.setComponentID("harmonic");
    harmonicChoice.addItemList(dynamic_cast<juce::AudioParameterChoice*>(p.state.getParameter("harmonic"))->choices,1);
    operationChoice.setName("Switch mode"); operationChoice.setComponentID("operation");
    operationChoice.addItemList({"Latching","Momentary"},1);
    addAndMakeVisible(harmonicChoice); addAndMakeVisible(operationChoice);
    harmonicAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"harmonic",harmonicChoice);
    operationAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"operation",operationChoice);
    operationChoice.onChange=[this] { if(operationChoice.getSelectedId()==2) { held=false; setEngaged(false); } };
    harmonicLabel.setText("HARMONIC",juce::dontSendNotification); operationLabel.setText("FOOTSWITCH",juce::dontSendNotification);
    for(auto* label: {&harmonicLabel,&operationLabel}) {
        label->setFont(font(11,true)); label->setColour(juce::Label::textColourId,muted); addAndMakeVisible(*label);
    }
    feedback.setName("Feedback footswitch"); feedback.setComponentID("feedback"); addAndMakeVisible(feedback);
    feedback.onStateChange=[this] {
        if(audioProcessor.state.getRawParameterValue("operation")->load()>0.5f) {
            const bool down=feedback.getState()==juce::Button::buttonDown;
            if(down!=held) { held=down; setEngaged(down); }
        }
    };
    feedback.onClick=[this] {
        if(audioProcessor.state.getRawParameterValue("operation")->load()<0.5f)
            setEngaged(audioProcessor.state.getRawParameterValue("engaged")->load()<0.5f);
    };
    bypass.setButtonText("BYPASS"); bypass.setComponentID("bypass"); bypass.setClickingTogglesState(true); addAndMakeVisible(bypass);
    bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",bypass);
    status.setName("Control help and status"); status.setComponentID("status"); status.setFont(font(12));
    status.setColour(juce::Label::textColourId,muted); status.setMinimumHorizontalScale(1); addAndMakeVisible(status);
    note.setFont(font(15,true)); note.setJustificationType(juce::Justification::centred); note.setColour(juce::Label::textColourId,lime); addAndMakeVisible(note);
    addMouseListener(this,true); setSize(820,580); timerCallback(); startTimerHz(24);
}
FeetBackEditor::~FeetBackEditor() {
    stopTimer(); if(held) setEngaged(false); removeMouseListener(this); setLookAndFeel(nullptr);
}
void FeetBackEditor::setEngaged(bool enabled) {
    auto* parameter=audioProcessor.state.getParameter("engaged");
    if((parameter->getValue()>0.5f)==enabled) return;
    parameter->beginChangeGesture(); parameter->setValueNotifyingHost(enabled ? 1.0f : 0.0f); parameter->endChangeGesture();
}
void FeetBackEditor::resized() {
    bypass.setBounds(681,30,107,33);
    for(size_t i=0;i<knobs.size();++i) {
        const int x=292+static_cast<int>(i%3)*164, y=110+static_cast<int>(i/3)*166;
        labels[i].setBounds(x,y,152,22); knobs[i].setBounds(x+9,y+25,134,132);
    }
    harmonicLabel.setBounds(299,446,295,20); harmonicChoice.setBounds(300,468,295,34);
    operationLabel.setBounds(619,446,165,20); operationChoice.setBounds(619,468,165,34);
    feedback.setBounds(45,440,202,62); note.setBounds(45,377,202,29);
    status.setBounds(29,530,760,34);
}
void FeetBackEditor::paint(juce::Graphics& g) {
    g.fillAll(background);
    g.setColour(panel); g.fillRoundedRectangle(27,104,238,410,16);
    g.setColour(lime); g.setFont(font(41,true)); g.drawText("FeetBack",31,24,258,52,juce::Justification::left);
    g.setFont(font(11.5f)); g.setColour(muted); g.drawText("HARMONIC FEEDBACK  /  NIKO AUDIO",296,38,368,24,juce::Justification::left);
    g.setColour(muted.withAlpha(0.18f)); g.drawHorizontalLine(88,32,788); g.drawHorizontalLine(522,32,788);
    if(feet) feet->drawWithin(g,{37,111,218,239},juce::RectanglePlacement::centred,1);
    g.setColour(muted.withAlpha(0.18f)); g.fillRoundedRectangle(58,417,176,4,2);
    g.setColour(lime); g.fillRoundedRectangle(58,417,176*displayEnvelope,4,2);
}
void FeetBackEditor::timerCallback() {
    const bool momentary=audioProcessor.state.getRawParameterValue("operation")->load()>0.5f;
    const bool on=audioProcessor.state.getRawParameterValue("engaged")->load()>0.5f;
    feedback.setButtonText(momentary ? "HOLD FOR FEEDBACK" : on ? "FEEDBACK ON" : "FEEDBACK OFF");
    feedback.setToggleState(on,juce::dontSendNotification);
    const float hz=audioProcessor.pitch.load();
    if(hz>0) {
        const int midi=juce::roundToInt(69+12*std::log2(hz/440.0f));
        note.setText(juce::MidiMessage::getMidiNoteName(midi,true,true,3)+"   /   "+juce::String(juce::roundToInt(hz))+" Hz",juce::dontSendNotification);
    } else note.setText("PLAY A HELD NOTE",juce::dontSendNotification);
    displayEnvelope=audioProcessor.envelope.load(); refreshStatus(); repaint(39,409,210,19);
}
void FeetBackEditor::describe(juce::Component* c) {
    juce::String text;
    for(size_t i=0;i<knobs.size();++i)
        if(c==&knobs[i] || knobs[i].isParentOf(c) || c==&labels[i]) text=descriptions[i];
    if(c==&harmonicChoice || harmonicChoice.isParentOf(c) || c==&harmonicLabel)
        text="Choose a harmonic. Auto low/high selects by note register; single notes track best.";
    if(c==&operationChoice || operationChoice.isParentOf(c) || c==&operationLabel)
        text="Latching toggles feedback on/off. Momentary keeps it on while the feedback button is held.";
    if(c==&feedback) text="Engage feedback on a held note. Releasing it uses the Release time; Bypass is immediate dry.";
    if(c==&bypass) text="Bypass passes the original signal unchanged, with no added delay.";
    if(help!=text) { help=text; refreshStatus(); }
}
void FeetBackEditor::refreshStatus() {
    juce::String text=help;
    if(text.isEmpty()) {
        if(audioProcessor.state.getRawParameterValue("bypass")->load()>0.5f) text="BYPASSED  /  Original guitar signal";
        else if(audioProcessor.envelope.load()>0.01f) text="LET IT SING  /  Feedback follows the note and its bends";
        else text="Place before your amp sim. Play a single note and let it bloom.";
    }
    status.setText(text,juce::dontSendNotification);
}
void FeetBackEditor::mouseEnter(const juce::MouseEvent& e) { describe(e.eventComponent); }
void FeetBackEditor::mouseMove(const juce::MouseEvent& e) { describe(e.eventComponent); }
void FeetBackEditor::mouseExit(const juce::MouseEvent& e) { describe(getComponentAt(e.getEventRelativeTo(this).getPosition())); }
