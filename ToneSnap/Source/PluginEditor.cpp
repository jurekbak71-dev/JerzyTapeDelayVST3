#include "PluginEditor.h"

namespace
{
const juce::Colour background { 13, 16, 23 };
const juce::Colour panel { 22, 27, 37 };
const juce::Colour panelRaised { 29, 36, 48 };
const juce::Colour accent { 85, 231, 206 };
const juce::Colour textMain { 237, 243, 249 };
const juce::Colour textMuted { 130, 145, 161 };

void setCaption(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(11.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, textMuted);
    label.setJustificationType(juce::Justification::centredLeft);
}

void styleCombo(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId, panelRaised);
    box.setColour(juce::ComboBox::textColourId, textMain);
    box.setColour(juce::ComboBox::outlineColourId, juce::Colour(57, 69, 86));
    box.setColour(juce::ComboBox::arrowColourId, accent);
    box.setColour(juce::PopupMenu::backgroundColourId, panelRaised);
    box.setColour(juce::PopupMenu::textColourId, textMain);
    box.setColour(juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha(0.28f));
    box.setColour(juce::PopupMenu::highlightedTextColourId, textMain);
}

void styleSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 20);
    slider.setRange(0.0, 100.0, 1.0);
    slider.setTextValueSuffix("%");
    slider.setNumDecimalPlacesToDisplay(0);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(54, 67, 83));
    slider.setColour(juce::Slider::thumbColourId, textMain);
    slider.setColour(juce::Slider::textBoxTextColourId, textMain);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, panel);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}
}

ToneSnapAudioProcessorEditor::ToneSnapAudioProcessorEditor(ToneSnapAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(620, 410);
    setResizable(false, false);

    const juce::StringArray notes { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const juce::StringArray scales { "Chromatic", "Major", "Minor" };
    keyBox.addItemList(notes, 1);
    scaleBox.addItemList(scales, 1);
    styleCombo(keyBox);
    styleCombo(scaleBox);

    setCaption(keyLabel, "KEY");
    setCaption(scaleLabel, "SCALE");
    setCaption(retuneLabel, "RETUNE");
    setCaption(amountLabel, "AMOUNT");
    setCaption(mixLabel, "MIX");
    const std::array<juce::Component*, 10> controls {
        &keyLabel, &scaleLabel, &retuneLabel, &amountLabel, &mixLabel,
        &keyBox, &scaleBox, &retuneSlider, &amountSlider, &mixSlider
    };
    for (auto* component : controls)
        addAndMakeVisible(component);

    styleSlider(retuneSlider);
    styleSlider(amountSlider);
    styleSlider(mixSlider);
    keyAttachment = std::make_unique<ComboAttachment>(processor.parameters, "key", keyBox);
    scaleAttachment = std::make_unique<ComboAttachment>(processor.parameters, "scale", scaleBox);
    retuneAttachment = std::make_unique<SliderAttachment>(processor.parameters, "speed", retuneSlider);
    amountAttachment = std::make_unique<SliderAttachment>(processor.parameters, "amount", amountSlider);
    mixAttachment = std::make_unique<SliderAttachment>(processor.parameters, "mix", mixSlider);
}

void ToneSnapAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    juce::ColourGradient backdrop(juce::Colour(24, 34, 45), 0.0f, 0.0f,
                                  background, 620.0f, 410.0f, true);
    g.setGradientFill(backdrop);
    g.fillAll();

    g.setColour(accent.withAlpha(0.95f));
    g.fillRoundedRectangle(25.0f, 26.0f, 4.0f, 35.0f, 2.0f);
    g.setColour(textMain);
    g.setFont(juce::Font(25.0f, juce::Font::bold));
    g.drawText("JERZY AUTO TUNE", 42, 20, 330, 31, juce::Justification::centredLeft);
    g.setColour(textMuted);
    g.setFont(juce::Font(10.5f, juce::Font::bold));
    g.drawText("REAL-TIME VOCAL TUNING", 43, 49, 260, 18, juce::Justification::centredLeft);

    auto badge = juce::Rectangle<float>(473.0f, 29.0f, 118.0f, 27.0f);
    g.setColour(panelRaised);
    g.fillRoundedRectangle(badge, 13.5f);
    g.setColour(accent);
    g.fillEllipse(485.0f, 39.0f, 7.0f, 7.0f);
    g.setColour(textMuted);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("PITCH CORRECT", 498, 34, 86, 17, juce::Justification::centredLeft);

    g.setColour(panel.withAlpha(0.96f));
    g.fillRoundedRectangle(22.0f, 83.0f, 576.0f, 302.0f, 18.0f);
    g.setColour(juce::Colour(48, 61, 78));
    g.drawRoundedRectangle(22.0f, 83.0f, 576.0f, 302.0f, 18.0f, 1.0f);

    g.setColour(accent.withAlpha(0.7f));
    g.fillRoundedRectangle(42.0f, 178.0f, 536.0f, 1.0f, 0.5f);

    g.setColour(textMuted);
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText("CORRECTION", 42, 191, 160, 16, juce::Justification::centredLeft);
}

void ToneSnapAudioProcessorEditor::resized()
{
    keyLabel.setBounds(42, 101, 230, 17);
    keyBox.setBounds(42, 122, 230, 39);
    scaleLabel.setBounds(296, 101, 282, 17);
    scaleBox.setBounds(296, 122, 282, 39);

    retuneLabel.setJustificationType(juce::Justification::centred);
    amountLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setJustificationType(juce::Justification::centred);
    retuneLabel.setBounds(49, 217, 145, 17);
    amountLabel.setBounds(237, 217, 145, 17);
    mixLabel.setBounds(425, 217, 145, 17);

    retuneSlider.setBounds(49, 232, 145, 139);
    amountSlider.setBounds(237, 232, 145, 139);
    mixSlider.setBounds(425, 232, 145, 139);
}
