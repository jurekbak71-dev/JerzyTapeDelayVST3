#include "PluginEditor.h"

namespace
{
const juce::Colour outer { 18, 21, 20 };
const juce::Colour metal { 202, 190, 164 };
const juce::Colour face { 185, 170, 140 };
const juce::Colour faceDark { 160, 147, 121 };
const juce::Colour ink { 61, 56, 46 };
const juce::Colour brass { 203, 177, 112 };
const juce::Colour cream { 235, 224, 198 };
const juce::Colour inkPanel { 27, 31, 29 };
const juce::Colour red { 177, 76, 54 };
const std::array<juce::String, 12> noteLabels {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
const std::array<const char*, 12> noteParameterIds {
    "noteC", "noteCs", "noteD", "noteDs", "noteE", "noteF",
    "noteFs", "noteG", "noteGs", "noteA", "noteAs", "noteB"
};

void addLabel(juce::Label& label, const juce::String& text, float size = 11.0f,
              juce::Colour colour = ink, bool centred = false)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(size, juce::Font::bold));
    label.setColour(juce::Label::textColourId, colour);
    label.setJustificationType(centred ? juce::Justification::centred : juce::Justification::centredLeft);
}

void drawScrew(juce::Graphics& g, float x, float y)
{
    g.setColour(juce::Colour(134, 123, 99));
    g.fillEllipse(x - 8.0f, y - 8.0f, 16.0f, 16.0f);
    g.setColour(juce::Colour(76, 69, 55));
    g.drawEllipse(x - 8.0f, y - 8.0f, 16.0f, 16.0f, 1.4f);
    g.drawLine(x - 4.0f, y, x + 4.0f, y, 1.8f);
}

void drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour fill = face)
{
    g.setColour(juce::Colour(87, 78, 62).withAlpha(0.38f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 2.0f), 5.0f);
    g.setColour(fill);
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(juce::Colour(111, 100, 78));
    g.drawRoundedRectangle(bounds, 5.0f, 1.2f);
}

void drawSectionTitle(juce::Graphics& g, const juce::String& title, float x, float y, float width)
{
    g.setColour(ink);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(title, juce::Rectangle<float>(x, y, width, 20.0f), juce::Justification::centredLeft);
}
}

ToneSnapAudioProcessorEditor::ToneSnapAudioProcessorEditor(ToneSnapAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1100, 705);
    setResizable(true, true);
    setResizeLimits(960, 615, 1280, 820);

    keyBox.addItemList({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    scaleBox.addItemList({ "Chromatic", "Major", "Minor" }, 1);
    for (auto* combo : { &keyBox, &scaleBox })
    {
        combo->setColour(juce::ComboBox::backgroundColourId, inkPanel);
        combo->setColour(juce::ComboBox::textColourId, cream);
        combo->setColour(juce::ComboBox::outlineColourId, juce::Colour(90, 81, 64));
        combo->setColour(juce::ComboBox::arrowColourId, brass);
        combo->setColour(juce::PopupMenu::backgroundColourId, inkPanel);
        combo->setColour(juce::PopupMenu::textColourId, cream);
        combo->setColour(juce::PopupMenu::highlightedBackgroundColourId, brass.withAlpha(0.45f));
    }

    for (size_t i = 0; i < noteButtons.size(); ++i)
    {
        auto& button = noteButtons[i];
        const bool isAccidental = noteLabels[i].containsChar('#');
        button.setButtonText(noteLabels[i]);
        button.setClickingTogglesState(true);
        button.setColour(juce::TextButton::buttonColourId, isAccidental ? inkPanel : cream);
        button.setColour(juce::TextButton::buttonOnColourId, brass);
        button.setColour(juce::TextButton::textColourOffId, isAccidental ? cream : ink);
        button.setColour(juce::TextButton::textColourOnId, inkPanel);
        button.setColour(juce::TextButton::outlineColourId, juce::Colour(95, 84, 65));
        addAndMakeVisible(button);
    }

    configureSlider(speedSlider, " ms");
    configureSlider(amountSlider, " %");
    configureSlider(mixSlider, " %");
    configureSlider(thresholdSlider, " dB");
    configureSlider(ratioSlider, ":1");
    configureSlider(makeupSlider, " dB");
    configureSlider(lowSlider, " dB");
    configureSlider(midSlider, " dB");
    configureSlider(highSlider, " dB");
    configureSlider(outputSlider, " dB");

    const std::array<std::pair<juce::Label*, juce::String>, 12> captions {{
        { &keyLabel, "KEY" }, { &scaleLabel, "SCALE" }, { &speedLabel, "SPEED" },
        { &amountLabel, "AMOUNT" }, { &mixLabel, "MIX" }, { &thresholdLabel, "THRESHOLD" },
        { &ratioLabel, "RATIO" }, { &makeupLabel, "MAKEUP" }, { &lowLabel, "LOW" },
        { &midLabel, "MID" }, { &highLabel, "HIGH" }, { &outputLabel, "OUTPUT" }
    }};
    for (auto [label, text] : captions)
    {
        addLabel(*label, text, 9.0f, ink, true);
        addAndMakeVisible(label);
    }

    compressorButton.setClickingTogglesState(true);
    equalizerButton.setClickingTogglesState(true);
    for (auto* button : { &compressorButton, &equalizerButton })
    {
        button->setColour(juce::ToggleButton::textColourId, cream);
        button->setColour(juce::ToggleButton::tickColourId, brass);
        button->setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(91, 87, 75));
        button->setColour(juce::ToggleButton::tickColourId, brass);
        button->setColour(juce::TextButton::buttonColourId, juce::Colour(38, 42, 39));
        button->setColour(juce::TextButton::buttonOnColourId, juce::Colour(88, 91, 69));
        addAndMakeVisible(button);
    }

    for (auto* combo : { &keyBox, &scaleBox }) addAndMakeVisible(combo);
    for (auto* slider : { &speedSlider, &amountSlider, &mixSlider, &thresholdSlider, &ratioSlider,
                          &makeupSlider, &lowSlider, &midSlider, &highSlider, &outputSlider })
    {
        slider->setLookAndFeel(&analogLookAndFeel);
        addAndMakeVisible(slider);
    }

    auto& state = processor.parameters;
    keyAttachment = std::make_unique<ComboAttachment>(state, "key", keyBox);
    scaleAttachment = std::make_unique<ComboAttachment>(state, "scale", scaleBox);
    speedAttachment = std::make_unique<SliderAttachment>(state, "speed", speedSlider);
    amountAttachment = std::make_unique<SliderAttachment>(state, "amount", amountSlider);
    mixAttachment = std::make_unique<SliderAttachment>(state, "mix", mixSlider);
    thresholdAttachment = std::make_unique<SliderAttachment>(state, "compThreshold", thresholdSlider);
    ratioAttachment = std::make_unique<SliderAttachment>(state, "compRatio", ratioSlider);
    makeupAttachment = std::make_unique<SliderAttachment>(state, "compMakeup", makeupSlider);
    lowAttachment = std::make_unique<SliderAttachment>(state, "eqLow", lowSlider);
    midAttachment = std::make_unique<SliderAttachment>(state, "eqMid", midSlider);
    highAttachment = std::make_unique<SliderAttachment>(state, "eqHigh", highSlider);
    outputAttachment = std::make_unique<SliderAttachment>(state, "outputGain", outputSlider);
    compressorAttachment = std::make_unique<ButtonAttachment>(state, "compEnabled", compressorButton);
    equalizerAttachment = std::make_unique<ButtonAttachment>(state, "eqEnabled", equalizerButton);
    for (size_t i = 0; i < noteAttachments.size(); ++i)
        noteAttachments[i] = std::make_unique<ButtonAttachment>(state, noteParameterIds[i], noteButtons[i]);
    startTimerHz(30);
}

ToneSnapAudioProcessorEditor::~ToneSnapAudioProcessorEditor()
{
    for (auto* slider : { &speedSlider, &amountSlider, &mixSlider, &thresholdSlider, &ratioSlider,
                          &makeupSlider, &lowSlider, &midSlider, &highSlider, &outputSlider })
        slider->setLookAndFeel(nullptr);
}

void ToneSnapAudioProcessorEditor::configureSlider(juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 18);
    slider.setTextValueSuffix(suffix);
    slider.setNumDecimalPlacesToDisplay(1);
    slider.setColour(juce::Slider::textBoxTextColourId, cream);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, inkPanel);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

juce::Rectangle<int> ToneSnapAudioProcessorEditor::scaledBounds(float x, float y, float width, float height) const
{
    const float scale = juce::jmin(static_cast<float>(getWidth()) / 1280.0f,
                                   static_cast<float>(getHeight()) / 820.0f);
    return { juce::roundToInt(x * scale), juce::roundToInt(y * scale),
             juce::roundToInt(width * scale), juce::roundToInt(height * scale) };
}

void ToneSnapAudioProcessorEditor::resized()
{
    keyLabel.setBounds(scaledBounds(103, 239, 97, 18));
    scaleLabel.setBounds(scaledBounds(103, 295, 210, 14));
    keyBox.setBounds(scaledBounds(103, 262, 97, 38));
    scaleBox.setBounds(scaledBounds(103, 310, 210, 36));
    for (size_t i = 0; i < noteButtons.size(); ++i)
    {
        const int row = static_cast<int>(i / 6);
        const int column = static_cast<int>(i % 6);
        noteButtons[i].setBounds(scaledBounds(103.0f + column * 34.0f, 378.0f + row * 29.0f, 30.0f, 25.0f));
    }

    speedSlider.setBounds(scaledBounds(423, 282, 154, 151));
    amountSlider.setBounds(scaledBounds(670, 290, 82, 85));
    mixSlider.setBounds(scaledBounds(670, 391, 82, 84));
    speedLabel.setBounds(scaledBounds(424, 437, 152, 16));
    amountLabel.setBounds(scaledBounds(665, 378, 92, 15));
    mixLabel.setBounds(scaledBounds(665, 477, 92, 15));

    thresholdSlider.setBounds(scaledBounds(142, 609, 76, 82));
    ratioSlider.setBounds(scaledBounds(277, 609, 76, 82));
    makeupSlider.setBounds(scaledBounds(412, 609, 76, 82));
    thresholdLabel.setBounds(scaledBounds(133, 696, 94, 18));
    ratioLabel.setBounds(scaledBounds(268, 696, 94, 18));
    makeupLabel.setBounds(scaledBounds(403, 696, 94, 18));
    compressorButton.setBounds(scaledBounds(527, 624, 64, 22));

    lowSlider.setBounds(scaledBounds(704, 609, 76, 82));
    midSlider.setBounds(scaledBounds(832, 609, 76, 82));
    highSlider.setBounds(scaledBounds(960, 609, 76, 82));
    outputSlider.setBounds(scaledBounds(1088, 609, 76, 82));
    lowLabel.setBounds(scaledBounds(695, 696, 94, 18));
    midLabel.setBounds(scaledBounds(823, 696, 94, 18));
    highLabel.setBounds(scaledBounds(951, 696, 94, 18));
    outputLabel.setBounds(scaledBounds(1079, 696, 94, 18));
    equalizerButton.setBounds(scaledBounds(1097, 576, 75, 20));
}

void ToneSnapAudioProcessorEditor::paint(juce::Graphics& g)
{
    const float scale = juce::jmin(static_cast<float>(getWidth()) / 1280.0f,
                                   static_cast<float>(getHeight()) / 820.0f);
    g.fillAll(outer);
    g.addTransform(juce::AffineTransform::scale(scale));

    g.setColour(juce::Colour(44, 47, 44));
    g.fillRoundedRectangle(22, 20, 1236, 780, 19.0f);
    g.setColour(juce::Colour(83, 84, 78));
    g.drawRoundedRectangle(22, 20, 1236, 780, 19.0f, 3.0f);
    g.setColour(metal);
    g.fillRoundedRectangle(37, 35, 1206, 750, 11.0f);
    g.setColour(juce::Colour(104, 94, 75));
    g.drawRoundedRectangle(37, 35, 1206, 750, 11.0f, 3.0f);
    drawScrew(g, 57, 55); drawScrew(g, 1223, 55); drawScrew(g, 57, 765); drawScrew(g, 1223, 765);

    g.setColour(inkPanel);
    g.fillRoundedRectangle(67, 66, 1146, 86, 5.0f);
    g.setColour(brass);
    g.setFont(juce::Font(16.0f, juce::Font::plain));
    g.drawText("JERZY AUDIO", 91, 83, 185, 24, juce::Justification::centredLeft);
    g.setColour(juce::Colour(165, 160, 145));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("STUDIO SERIES  ·  MODEL 01", 91, 111, 190, 20, juce::Justification::centredLeft);
    g.setColour(juce::Colour(94, 88, 72));
    g.drawLine(288, 82, 288, 136, 1.0f);
    g.setColour(cream);
    g.setFont(juce::Font(34.0f, juce::Font::plain));
    g.drawText("JERZY AUTO TUNE", 355, 81, 790, 54, juce::Justification::centred);
    g.setColour(brass);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("VOCAL PROCESSOR", 1010, 81, 165, 20, juce::Justification::centredRight);
    g.setColour(juce::Colour(160, 160, 147));
    g.drawText("ANALOG-INSPIRED  ·  VST3", 1000, 108, 175, 20, juce::Justification::centredRight);

    drawPanel(g, { 67, 170, 1146, 330 }, face);
    drawSectionTitle(g, "01  ·  PITCH CORRECTION", 87, 184, 500);
    g.setColour(juce::Colour(138, 126, 101)); g.drawLine(87, 208, 1193, 208, 1.0f);
    drawPanel(g, { 87, 224, 244, 249 }, faceDark);
    drawPanel(g, { 349, 224, 474, 249 }, faceDark);
    drawPanel(g, { 841, 224, 352, 249 }, faceDark);
    drawSectionTitle(g, "KEY / SCALE", 103, 229, 210);
    drawSectionTitle(g, "RETUNE RESPONSE", 367, 229, 240);
    drawSectionTitle(g, "OUTPUT / MONITOR", 860, 229, 270);

    g.setColour(ink);
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText("NOTE FILTER  ·  CLICK TO ENABLE / DISABLE", 103, 357, 220, 17, juce::Justification::centredLeft);
    juce::String allowedNotes;
    for (size_t i = 0; i < noteLabels.size(); ++i)
        if (processor.parameters.getRawParameterValue(noteParameterIds[i])->load() >= 0.5f)
        {
            if (!allowedNotes.isEmpty()) allowedNotes += "  ";
            allowedNotes += noteLabels[i];
        }
    g.setColour(ink); g.setFont(juce::Font(8.0f, juce::Font::bold));
    g.drawText(allowedNotes.isEmpty() ? "NO NOTES ENABLED — CORRECTION BYPASSED" : "ENABLED: " + allowedNotes,
               103, 439, 220, 18, juce::Justification::centredLeft, true);

    g.setColour(ink);
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText("SLOW", 426, 287, 50, 16, juce::Justification::centred);
    g.drawText("FAST", 553, 287, 50, 16, juce::Justification::centred);
    g.setColour(juce::Colour(60, 56, 47));
    for (int i = 0; i <= 8; ++i)
    {
        const float angle = juce::MathConstants<float>::pi * (1.18f + 0.08f * i);
        const float cx = 500.0f, cy = 364.0f;
        const float r0 = 91.0f, r1 = (i % 2 == 0 ? 105.0f : 100.0f);
        g.drawLine(cx + std::cos(angle) * r0, cy + std::sin(angle) * r0,
                   cx + std::cos(angle) * r1, cy + std::sin(angle) * r1, 2.0f);
    }

    // Warm analog VU meter with live output needle.
    g.setColour(juce::Colour(232, 223, 203));
    g.fillRoundedRectangle(861, 265, 311, 117, 5.0f);
    g.setColour(juce::Colour(92, 82, 65));
    g.drawRoundedRectangle(861, 265, 311, 117, 5.0f, 2.0f);
    juce::Path arc;
    arc.startNewSubPath(887, 353);
    arc.quadraticTo(1017, 231, 1147, 353);
    g.setColour(juce::Colour(47, 50, 45)); g.strokePath(arc, juce::PathStrokeType(2.0f));
    g.setFont(juce::Font(8.0f, juce::Font::bold));
    const std::array<juce::String, 7> dbLabels { "-20", "-10", "-7", "-5", "-3", "0", "+3" };
    const std::array<juce::Point<float>, 7> dbPositions {{ { 892, 336 }, { 934, 307 }, { 980, 289 },
                                                           { 1023, 282 }, { 1066, 289 }, { 1112, 307 }, { 1141, 336 } }};
    for (size_t i = 0; i < dbLabels.size(); ++i)
    {
        g.setColour(ink);
        g.drawText(dbLabels[i], static_cast<int>(dbPositions[i].x - 14), static_cast<int>(dbPositions[i].y - 6), 28, 13,
                   juce::Justification::centred);
    }
    const float outputDb = juce::jlimit(-20.0f, 3.0f,
        juce::Decibels::gainToDecibels(juce::jmax(processor.getOutputPeak(), 1.0e-5f)));
    const float meterAngle = juce::jmap(outputDb, -20.0f, 3.0f, -2.35f, -0.79f);
    const float needleX = 1020.0f + std::cos(meterAngle) * 74.0f;
    const float needleY = 352.0f + std::sin(meterAngle) * 74.0f;
    g.setColour(red); g.drawLine(1020, 352, needleX, needleY, 3.0f);
    g.setColour(ink); g.fillEllipse(1014, 346, 12, 12);
    g.setColour(ink); g.setFont(juce::Font(8.0f, juce::Font::bold));
    g.drawText("VU  ·  OUTPUT LEVEL", 934, 361, 170, 15, juce::Justification::centred);
    g.setColour(red); g.fillEllipse(875, 408, 12, 12);
    g.setColour(ink); g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText("SIGNAL", 891, 405, 65, 18, juce::Justification::centredLeft);
    g.setColour(processor.getInputPeak() > 0.0001f ? juce::Colour(108, 132, 74) : juce::Colour(91, 87, 74));
    g.fillEllipse(1000, 408, 12, 12);
    g.setColour(ink); g.drawText("TUNED", 1016, 405, 75, 18, juce::Justification::centredLeft);
    g.setColour(ink); g.drawText("OUTPUT", 875, 441, 63, 18, juce::Justification::centredLeft);
    g.setColour(inkPanel); g.fillRoundedRectangle(944, 434, 213, 25, 3.0f);
    g.setColour(cream); g.drawText(juce::String(juce::Decibels::gainToDecibels(juce::jmax(processor.getOutputPeak(), 1.0e-5f)), 1) + " dB",
                                    949, 437, 203, 18, juce::Justification::centred);

    drawPanel(g, { 67, 518, 1146, 230 }, face);
    drawSectionTitle(g, "02  ·  ANALOG OUTPUT CHAIN", 87, 531, 520);
    g.setColour(juce::Colour(138, 126, 101)); g.drawLine(87, 556, 1193, 556, 1.0f);
    drawPanel(g, { 87, 571, 528, 155 }, faceDark);
    drawPanel(g, { 635, 571, 558, 155 }, faceDark);
    g.setColour(inkPanel); g.fillRoundedRectangle(87, 571, 528, 28, 4.0f);
    g.setColour(inkPanel); g.fillRoundedRectangle(635, 571, 558, 28, 4.0f);
    g.setColour(red); g.fillEllipse(101, 581, 8, 8); g.fillEllipse(649, 581, 8, 8);
    g.setColour(cream); g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("ANALOG COMPRESSOR", 119, 576, 260, 19, juce::Justification::centredLeft);
    g.drawText("ANALOG EQUALIZER", 667, 576, 250, 19, juce::Justification::centredLeft);

    g.setColour(ink);
    g.setFont(juce::Font(8.0f, juce::Font::bold));
    g.drawText("JERZY AUDIO  ·  ANALOG-INSPIRED VOCAL PROCESSOR", 88, 758, 500, 14, juce::Justification::centredLeft);
    g.drawText("STUDIO SERIES  ·  VST3", 950, 758, 242, 14, juce::Justification::centredRight);
}

void ToneSnapAudioProcessorEditor::timerCallback()
{
    repaint();
}

void ToneSnapAudioProcessorEditor::AnalogLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
    float startAngle, float endAngle, juce::Slider&)
{
    const float diameter = static_cast<float>(juce::jmin(width, height)) - 12.0f;
    const float radius = diameter * 0.5f;
    const float centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float centreY = static_cast<float>(y) + radius + 4.0f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    g.setColour(juce::Colour(65, 57, 42).withAlpha(0.6f));
    g.fillEllipse(centreX - radius, centreY - radius + 3.0f, diameter, diameter);
    juce::ColourGradient knobGradient(juce::Colour(83, 86, 79), centreX, centreY - radius,
                                      juce::Colour(12, 15, 14), centreX, centreY + radius, false);
    g.setGradientFill(knobGradient);
    g.fillEllipse(centreX - radius, centreY - radius, diameter, diameter);
    g.setColour(juce::Colour(13, 16, 15));
    g.drawEllipse(centreX - radius, centreY - radius, diameter, diameter, 2.0f);
    g.setColour(juce::Colour(148, 143, 124).withAlpha(0.8f));
    g.drawEllipse(centreX - radius + 8.0f, centreY - radius + 8.0f,
                  diameter - 16.0f, diameter - 16.0f, 1.0f);
    const float pointerLength = radius * 0.72f;
    g.setColour(cream);
    g.drawLine(centreX, centreY,
               centreX + std::sin(angle) * pointerLength,
               centreY - std::cos(angle) * pointerLength, 3.0f);
    g.setColour(brass);
    g.fillEllipse(centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);
}
