#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class FXLook : public juce::LookAndFeel_V4
{
public:
    FXLook();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
    void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&,juce::Label&) override;
};

class FXKnob: public juce::Slider
{
public:
    void setNeutralValue(double v){ neutralValue=v; }
    void mouseDown(const juce::MouseEvent&e) override
    {
        if(e.mods.isRightButtonDown()){setValue(juce::jlimit(getMinimum(),getMaximum(),neutralValue),juce::sendNotificationSync);return;}
        juce::Slider::mouseDown(e);
    }
private:
    double neutralValue=0.0;
};

class JerzyFXBlockAudioProcessorEditor: public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit JerzyFXBlockAudioProcessorEditor(JerzyFXBlockAudioProcessor&);
    ~JerzyFXBlockAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    using SA=juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA=juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA=juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void knob(FXKnob&,const juce::String&,const juce::String& unit={}, double neutral=0.0);
    void tog(juce::ToggleButton&,const juce::String&,juce::Colour);
    void combo(juce::ComboBox&,const juce::StringArray&);
    void place(juce::Component&,float,float,float,float);
    int slotForEffect(int effectId) const;
    float slotX(int slot) const { return 15.0f + 220.0f*slot; }
    void updateOrderCaption();
    void updateSyncControls();
    void drawParamLabel(juce::Graphics&,const juce::String&,float,float,float) const;

    JerzyFXBlockAudioProcessor& proc;
    FXLook look;
    juce::Label title,sub;
    std::array<int,6> order {{0,1,2,3,4,5}};
    int dragSlot=-1;

    juce::ToggleButton revOn,delOn,choOn,widOn,rotOn,shOn,delSync,rotSync;
    juce::ComboBox delDivision,rotDivision;
    FXKnob revSize,revDamp,revMix,delTime,delFb,delMix,choRate,choDepth,choMix,width,widMix,rotRate,rotDepth,rotMix,shAmt,shMix;
    std::vector<std::unique_ptr<SA>> sa;
    std::vector<std::unique_ptr<BA>> ba;
    std::vector<std::unique_ptr<CA>> ca;
};
