#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class FXLook : public juce::LookAndFeel_V4
{
public:
    FXLook();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
    juce::Font getLabelFont(juce::Label&) override;
};
class FXKnob: public juce::Slider
{
public:
    void mouseDown(const juce::MouseEvent&e) override
    {
        if(e.mods.isRightButtonDown()){setValue(getMinimum()<=0&&getMaximum()>=0?0:getMinimum(),juce::sendNotificationSync);return;}
        juce::Slider::mouseDown(e);
    }
};
class JerzyFXBlockAudioProcessorEditor: public juce::AudioProcessorEditor
{
public:
    explicit JerzyFXBlockAudioProcessorEditor(JerzyFXBlockAudioProcessor&);
    ~JerzyFXBlockAudioProcessorEditor() override;
    void paint(juce::Graphics&) override; void resized() override;
private:
    using SA=juce::AudioProcessorValueTreeState::SliderAttachment; using BA=juce::AudioProcessorValueTreeState::ButtonAttachment;
    void knob(FXKnob&,const juce::String&,const juce::String&={}); void tog(juce::ToggleButton&,const juce::String&,juce::Colour);
    void place(juce::Component&,float,float,float,float);
    JerzyFXBlockAudioProcessor& proc; FXLook look;
    juce::Label title,sub;
    juce::ToggleButton revOn,delOn,choOn,widOn,rotOn,shOn;
    FXKnob revSize,revDamp,revMix,delTime,delFb,delMix,choRate,choDepth,choMix,width,widMix,rotRate,rotDepth,rotMix,shAmt,shMix;
    std::vector<std::unique_ptr<SA>> sa; std::vector<std::unique_ptr<BA>> ba;
};
