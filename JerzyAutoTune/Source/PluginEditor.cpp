#include "PluginEditor.h"

namespace
{
const juce::Colour outer(25, 27, 25);
const juce::Colour metal(177, 169, 148);
const juce::Colour face(157, 148, 126);
const juce::Colour faceDark(132, 124, 106);
const juce::Colour panel(39, 42, 39);
const juce::Colour ink(27, 29, 27);
const juce::Colour cream(229, 221, 198);
const juce::Colour brass(183, 145, 65);
const juce::Colour red(154, 55, 42);
const juce::StringArray noteNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
const std::array<const char*, 12> noteParameterIds {
    "noteC", "noteCs", "noteD", "noteDs", "noteE", "noteF",
    "noteFs", "noteG", "noteGs", "noteA", "noteAs", "noteB"
};

void addComboItems(juce::ComboBox& box, const juce::StringArray& items)
{
    for (int i = 0; i < items.size(); ++i)
        box.addItem(items[i], i + 1);
}
}

JerzyAutoTuneAudioProcessorEditor::JerzyAutoTuneAudioProcessorEditor(JerzyAutoTuneAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setResizable(true, true);
    setResizeLimits(1024, 720, 1920, 1350);
    setSize(1280, 900);

    addComboItems(keyBox, noteNames);
    addComboItems(scaleBox, { "Chromatic", "Major", "Minor" });
    for (auto* c : { &keyBox, &scaleBox })
    {
        c->setColour(juce::ComboBox::backgroundColourId, panel);
        c->setColour(juce::ComboBox::textColourId, cream);
        c->setColour(juce::ComboBox::outlineColourId, brass.withAlpha(0.6f));
        addAndMakeVisible(c);
    }

    configureLabel(keyLabel, "KEY");
    configureLabel(scaleLabel, "SCALE");
    configureLabel(speedLabel, "SPEED");
    configureLabel(amountLabel, "AMOUNT");
    configureLabel(mixLabel, "MIX");

    configureSlider(speedSlider, " ms");
    configureSlider(amountSlider, " %");
    configureSlider(mixSlider, " %");
    addAndMakeVisible(speedSlider);
    addAndMakeVisible(amountSlider);
    addAndMakeVisible(mixSlider);

    for (size_t i = 0; i < noteButtons.size(); ++i)
    {
        auto& b = noteButtons[i];
        b.setButtonText(noteNames[static_cast<int>(i)]);
        b.setClickingTogglesState(true);
        b.setColour(juce::TextButton::buttonColourId, panel);
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(89, 111, 70));
        b.setColour(juce::TextButton::textColourOffId, cream);
        b.setColour(juce::TextButton::textColourOnId, cream);
        addAndMakeVisible(b);
    }

    configureSlider(gateThreshold, " dB");
    configureSlider(gateRelease, " ms");
    configureLabel(gateThresholdLabel, "THRESH");
    configureLabel(gateReleaseLabel, "RELEASE");
    gateModule.sliders = { &gateThreshold, &gateRelease, nullptr, nullptr };
    gateModule.labels = { &gateThresholdLabel, &gateReleaseLabel, nullptr, nullptr };
    gateModule.count = 2;

    configureSlider(noiseThreshold, " dB");
    configureSlider(noiseReduction, " dB");
    configureLabel(noiseThresholdLabel, "FLOOR");
    configureLabel(noiseReductionLabel, "REDUCE");
    noiseModule.sliders = { &noiseThreshold, &noiseReduction, nullptr, nullptr };
    noiseModule.labels = { &noiseThresholdLabel, &noiseReductionLabel, nullptr, nullptr };
    noiseModule.count = 2;

    configureSlider(deEssFreq, " Hz");
    configureSlider(deEssAmount, " dB");
    configureLabel(deEssFreqLabel, "FREQ");
    configureLabel(deEssAmountLabel, "AMOUNT");
    deEssModule.sliders = { &deEssFreq, &deEssAmount, nullptr, nullptr };
    deEssModule.labels = { &deEssFreqLabel, &deEssAmountLabel, nullptr, nullptr };
    deEssModule.count = 2;

    configureSlider(satDrive, " dB");
    configureSlider(satMix, " %");
    configureLabel(satDriveLabel, "DRIVE");
    configureLabel(satMixLabel, "MIX");
    satModule.sliders = { &satDrive, &satMix, nullptr, nullptr };
    satModule.labels = { &satDriveLabel, &satMixLabel, nullptr, nullptr };
    satModule.count = 2;

    configureSlider(doublerAmount, " %");
    configureSlider(doublerDelay, " ms");
    configureLabel(doublerAmountLabel, "AMOUNT");
    configureLabel(doublerDelayLabel, "DELAY");
    doublerModule.sliders = { &doublerAmount, &doublerDelay, nullptr, nullptr };
    doublerModule.labels = { &doublerAmountLabel, &doublerDelayLabel, nullptr, nullptr };
    doublerModule.count = 2;

    configureSlider(compThreshold, " dB");
    configureSlider(compRatio, ":1");
    configureSlider(compMakeup, " dB");
    configureSlider(limiterCeiling, " dB");
    configureLabel(compThresholdLabel, "THRESH");
    configureLabel(compRatioLabel, "RATIO");
    configureLabel(compMakeupLabel, "MAKEUP");
    configureLabel(limiterCeilingLabel, "LIMIT");
    compModule.sliders = { &compThreshold, &compRatio, &compMakeup, &limiterCeiling };
    compModule.labels = { &compThresholdLabel, &compRatioLabel, &compMakeupLabel, &limiterCeilingLabel };
    compModule.count = 4;

    configureSlider(eqLow, " dB");
    configureSlider(eqMid, " dB");
    configureSlider(eqHigh, " dB");
    configureSlider(outputGain, " dB");
    configureLabel(eqLowLabel, "BODY");
    configureLabel(eqMidLabel, "PRESENCE");
    configureLabel(eqHighLabel, "AIR");
    configureLabel(outputGainLabel, "OUTPUT");
    eqModule.sliders = { &eqLow, &eqMid, &eqHigh, &outputGain };
    eqModule.labels = { &eqLowLabel, &eqMidLabel, &eqHighLabel, &outputGainLabel };
    eqModule.count = 4;

    for (auto* module : { &gateModule, &noiseModule, &deEssModule, &satModule, &doublerModule, &compModule, &eqModule })
    {
        module->enabled.setColour(juce::ToggleButton::textColourId, cream);
        module->enabled.setColour(juce::ToggleButton::tickColourId, brass);
        addAndMakeVisible(module->enabled);
        module->enabled.setVisible(true);
        module->enabled.toFront(false);
    }

    const std::array<juce::Slider*, 18> visibleVocalSliders {
        &gateThreshold, &gateRelease,
        &noiseThreshold, &noiseReduction,
        &deEssFreq, &deEssAmount,
        &satDrive, &satMix,
        &doublerAmount, &doublerDelay,
        &compThreshold, &compRatio, &compMakeup, &limiterCeiling,
        &eqLow, &eqMid, &eqHigh, &outputGain
    };
    for (auto* slider : visibleVocalSliders)
    {
        addAndMakeVisible(*slider);
        slider->setVisible(true);
        slider->toFront(false);
    }

    const std::array<juce::Label*, 18> visibleVocalLabels {
        &gateThresholdLabel, &gateReleaseLabel,
        &noiseThresholdLabel, &noiseReductionLabel,
        &deEssFreqLabel, &deEssAmountLabel,
        &satDriveLabel, &satMixLabel,
        &doublerAmountLabel, &doublerDelayLabel,
        &compThresholdLabel, &compRatioLabel, &compMakeupLabel, &limiterCeilingLabel,
        &eqLowLabel, &eqMidLabel, &eqHighLabel, &outputGainLabel
    };
    for (auto* label : visibleVocalLabels)
    {
        addAndMakeVisible(*label);
        label->setVisible(true);
        label->toFront(false);
    }

    auto& state = processor.parameters;
    keyAttachment = std::make_unique<ComboAttachment>(state, "key", keyBox);
    scaleAttachment = std::make_unique<ComboAttachment>(state, "scale", scaleBox);
    speedAttachment = std::make_unique<SliderAttachment>(state, "speed", speedSlider);
    amountAttachment = std::make_unique<SliderAttachment>(state, "amount", amountSlider);
    mixAttachment = std::make_unique<SliderAttachment>(state, "mix", mixSlider);
    for (size_t i = 0; i < noteAttachments.size(); ++i)
        noteAttachments[i] = std::make_unique<ButtonAttachment>(state, noteParameterIds[i], noteButtons[i]);

    const std::array<const char*, 18> sliderIds {
        "gateThreshold", "gateRelease",
        "noiseThreshold", "noiseReduction",
        "deEsserFreq", "deEsserAmount",
        "satDrive", "satMix",
        "doublerAmount", "doublerDelay",
        "compThreshold", "compRatio", "compMakeup", "limiterCeiling",
        "eqLow", "eqMid", "eqHigh", "outputGain"
    };
    const std::array<juce::Slider*, 18> sliderPtrs {
        &gateThreshold, &gateRelease,
        &noiseThreshold, &noiseReduction,
        &deEssFreq, &deEssAmount,
        &satDrive, &satMix,
        &doublerAmount, &doublerDelay,
        &compThreshold, &compRatio, &compMakeup, &limiterCeiling,
        &eqLow, &eqMid, &eqHigh, &outputGain
    };
    for (size_t i = 0; i < sliderIds.size(); ++i)
        vocalSliderAttachments[i] = std::make_unique<SliderAttachment>(state, sliderIds[i], *sliderPtrs[i]);

    const std::array<const char*, 7> buttonIds {
        "gateEnabled", "noiseEnabled", "deEsserEnabled", "satEnabled", "doublerEnabled", "compEnabled", "eqEnabled"
    };
    const std::array<juce::ToggleButton*, 7> buttonPtrs {
        &gateModule.enabled, &noiseModule.enabled, &deEssModule.enabled, &satModule.enabled,
        &doublerModule.enabled, &compModule.enabled, &eqModule.enabled
    };
    for (size_t i = 0; i < buttonIds.size(); ++i)
        vocalButtonAttachments[i] = std::make_unique<ButtonAttachment>(state, buttonIds[i], *buttonPtrs[i]);

    startTimerHz(30);
}

JerzyAutoTuneAudioProcessorEditor::~JerzyAutoTuneAudioProcessorEditor()
{
    std::array<juce::Slider*, 21> sliders {
        &speedSlider, &amountSlider, &mixSlider,
        &gateThreshold, &gateRelease, &noiseThreshold, &noiseReduction,
        &deEssFreq, &deEssAmount, &satDrive, &satMix,
        &doublerAmount, &doublerDelay, &compThreshold, &compRatio,
        &compMakeup, &limiterCeiling, &eqLow, &eqMid, &eqHigh, &outputGain
    };
    for (auto* s : sliders)
        s->setLookAndFeel(nullptr);
}

void JerzyAutoTuneAudioProcessorEditor::configureSlider(juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 66, 17);
    slider.setTextValueSuffix(suffix);
    slider.setNumDecimalPlacesToDisplay(1);
    slider.setLookAndFeel(&analogLookAndFeel);
    slider.setColour(juce::Slider::textBoxTextColourId, cream);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, panel);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void JerzyAutoTuneAudioProcessorEditor::configureLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, ink);
    label.setFont(juce::Font(10.0f, juce::Font::bold));
    addAndMakeVisible(label);
}

juce::Rectangle<int> JerzyAutoTuneAudioProcessorEditor::scaledBounds(float x, float y, float w, float h) const
{
    const float sx = static_cast<float>(getWidth()) / 1280.0f;
    const float sy = static_cast<float>(getHeight()) / 900.0f;
    const float s = juce::jmin(sx, sy);
    const float ox = (static_cast<float>(getWidth()) - 1280.0f * s) * 0.5f;
    const float oy = (static_cast<float>(getHeight()) - 900.0f * s) * 0.5f;
    return { juce::roundToInt(ox + x * s), juce::roundToInt(oy + y * s),
             juce::roundToInt(w * s), juce::roundToInt(h * s) };
}

void JerzyAutoTuneAudioProcessorEditor::layoutModule(ModuleControls& module, float x, float y, float w, float h)
{
    module.enabled.setBounds(scaledBounds(x + w - 54.0f, y + 8.0f, 46.0f, 20.0f));
    const float usableY = y + 38.0f;
    const float usableH = h - 44.0f;
    const float cellW = w / static_cast<float>(module.count);
    for (int i = 0; i < module.count; ++i)
    {
        const float cx = x + cellW * static_cast<float>(i);
        module.sliders[static_cast<size_t>(i)]->setBounds(scaledBounds(cx + 5.0f, usableY, cellW - 10.0f, usableH - 20.0f));
        module.labels[static_cast<size_t>(i)]->setBounds(scaledBounds(cx + 3.0f, y + h - 24.0f, cellW - 6.0f, 16.0f));
    }
}

void JerzyAutoTuneAudioProcessorEditor::resized()
{
    keyLabel.setBounds(scaledBounds(86, 187, 110, 18));
    keyBox.setBounds(scaledBounds(86, 207, 110, 33));
    scaleLabel.setBounds(scaledBounds(208, 187, 150, 18));
    scaleBox.setBounds(scaledBounds(208, 207, 150, 33));

    for (size_t i = 0; i < noteButtons.size(); ++i)
    {
        const int row = static_cast<int>(i / 6);
        const int col = static_cast<int>(i % 6);
        noteButtons[i].setBounds(scaledBounds(86.0f + col * 46.0f, 263.0f + row * 31.0f, 42.0f, 26.0f));
    }

    speedSlider.setBounds(scaledBounds(410, 190, 122, 128));
    amountSlider.setBounds(scaledBounds(548, 190, 100, 128));
    mixSlider.setBounds(scaledBounds(660, 190, 100, 128));
    speedLabel.setBounds(scaledBounds(420, 314, 102, 17));
    amountLabel.setBounds(scaledBounds(548, 314, 100, 17));
    mixLabel.setBounds(scaledBounds(660, 314, 100, 17));

    layoutModule(gateModule, 65, 405, 270, 185);
    layoutModule(noiseModule, 345, 405, 270, 185);
    layoutModule(deEssModule, 625, 405, 270, 185);
    layoutModule(satModule, 905, 405, 310, 185);

    layoutModule(doublerModule, 65, 605, 300, 205);
    layoutModule(compModule, 375, 605, 470, 205);
    layoutModule(eqModule, 855, 605, 360, 205);

    for (auto* slider : { &speedSlider, &amountSlider, &mixSlider })
        slider->toFront(false);

    for (auto* module : { &gateModule, &noiseModule, &deEssModule, &satModule, &doublerModule, &compModule, &eqModule })
    {
        module->enabled.toFront(false);
        for (int i = 0; i < module->count; ++i)
        {
            module->sliders[static_cast<size_t>(i)]->toFront(false);
            module->labels[static_cast<size_t>(i)]->toFront(false);
        }
    }
}

void JerzyAutoTuneAudioProcessorEditor::drawModule(juce::Graphics& g, const juce::String& title,
                                                   float x, float y, float w, float h, bool enabled)
{
    g.setColour(faceDark);
    g.fillRoundedRectangle(x, y, w, h, 7.0f);
    g.setColour(enabled ? brass : juce::Colour(96, 91, 79));
    g.drawRoundedRectangle(x, y, w, h, 7.0f, 2.0f);
    g.setColour(panel);
    g.fillRoundedRectangle(x, y, w, 32.0f, 7.0f);
    g.setColour(enabled ? cream : juce::Colour(148, 144, 130));
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(title, static_cast<int>(x + 12), static_cast<int>(y + 6),
               static_cast<int>(w - 80), 20, juce::Justification::centredLeft);
}

void JerzyAutoTuneAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(outer);
    const float sx = static_cast<float>(getWidth()) / 1280.0f;
    const float sy = static_cast<float>(getHeight()) / 900.0f;
    const float s = juce::jmin(sx, sy);
    const float ox = (static_cast<float>(getWidth()) - 1280.0f * s) * 0.5f;
    const float oy = (static_cast<float>(getHeight()) - 900.0f * s) * 0.5f;
    g.addTransform(juce::AffineTransform::translation(ox, oy).scaled(s));

    g.setColour(juce::Colour(46, 48, 45));
    g.fillRoundedRectangle(22, 20, 1236, 860, 18.0f);
    g.setColour(metal);
    g.fillRoundedRectangle(37, 35, 1206, 830, 11.0f);
    g.setColour(juce::Colour(104, 94, 75));
    g.drawRoundedRectangle(37, 35, 1206, 830, 11.0f, 2.0f);

    g.setColour(panel);
    g.fillRoundedRectangle(65, 62, 1150, 88, 6.0f);
    g.setColour(brass);
    g.setFont(juce::Font(16.0f, juce::Font::plain));
    g.drawText("JERZY AUDIO", 88, 81, 180, 22, juce::Justification::centredLeft);
    g.setColour(cream);
    g.setFont(juce::Font(35.0f, juce::Font::plain));
    g.drawText("JERZY AUTO TUNE", 320, 75, 650, 48, juce::Justification::centred);
    g.setColour(juce::Colour(160, 160, 147));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("PITCH + COMPLETE VOCAL CHAIN", 930, 87, 250, 20, juce::Justification::centredRight);
    g.drawText("VST3  -  WINDOWS x64", 930, 111, 250, 18, juce::Justification::centredRight);

    g.setColour(face);
    g.fillRoundedRectangle(65, 165, 1150, 205, 8.0f);
    g.setColour(ink);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("01  -  PITCH CORRECTION", 84, 172, 300, 22, juce::Justification::centredLeft);
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText("NOTE FILTER", 86, 244, 140, 16, juce::Justification::centredLeft);

    g.setColour(panel);
    g.fillRoundedRectangle(800, 190, 380, 136, 6.0f);
    g.setColour(cream);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("OUTPUT LEVEL", 822, 204, 120, 16, juce::Justification::centredLeft);
    const float level = juce::jlimit(0.0f, 1.0f, processor.getOutputPeak());
    g.setColour(juce::Colour(74, 78, 67));
    g.fillRoundedRectangle(822, 238, 332, 24, 4.0f);
    g.setColour(level > 0.9f ? red : brass);
    g.fillRoundedRectangle(822, 238, 332.0f * level, 24, 4.0f);
    const float outDb = juce::Decibels::gainToDecibels(juce::jmax(processor.getOutputPeak(), 1.0e-6f));
    g.setColour(cream);
    g.drawText(juce::String(outDb, 1) + " dB", 1010, 276, 144, 22, juce::Justification::centredRight);

    g.setColour(ink);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("02  -  VOCAL PROCESSING CHAIN", 65, 381, 420, 20, juce::Justification::centredLeft);

    drawModule(g, "GATE", 65, 405, 270, 185, gateModule.enabled.getToggleState());
    drawModule(g, "NOISE FILTER", 345, 405, 270, 185, noiseModule.enabled.getToggleState());
    drawModule(g, "DE-ESSER", 625, 405, 270, 185, deEssModule.enabled.getToggleState());
    drawModule(g, "SATURATION", 905, 405, 310, 185, satModule.enabled.getToggleState());
    drawModule(g, "DOUBLER", 65, 605, 300, 205, doublerModule.enabled.getToggleState());
    drawModule(g, "VOCAL COMPRESSOR + LIMITER", 375, 605, 470, 205, compModule.enabled.getToggleState());
    drawModule(g, "VOCAL EQ", 855, 605, 360, 205, eqModule.enabled.getToggleState());

    g.setColour(ink);
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText("CHAIN: GATE  >  NOISE FILTER  >  DE-ESSER  >  SATURATION  >  DOUBLER  >  COMP/LIMITER  >  VOCAL EQ",
               66, 828, 980, 18, juce::Justification::centredLeft);
    g.drawText("JERZY AUDIO  -  STUDIO SERIES", 1010, 828, 205, 18, juce::Justification::centredRight);
}

void JerzyAutoTuneAudioProcessorEditor::timerCallback()
{
    repaint();
}

void JerzyAutoTuneAudioProcessorEditor::AnalogLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
    float startAngle, float endAngle, juce::Slider&)
{
    const float d = static_cast<float>(juce::jmin(width, height)) - 22.0f;
    const float r = d * 0.5f;
    const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float cy = static_cast<float>(y) + r + 3.0f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    g.setColour(juce::Colour(52, 48, 40).withAlpha(0.5f));
    g.fillEllipse(cx - r, cy - r + 3.0f, d, d);
    juce::ColourGradient grad(juce::Colour(92, 94, 86), cx, cy - r,
                              juce::Colour(15, 17, 16), cx, cy + r, false);
    g.setGradientFill(grad);
    g.fillEllipse(cx - r, cy - r, d, d);
    g.setColour(juce::Colour(12, 14, 13));
    g.drawEllipse(cx - r, cy - r, d, d, 2.0f);
    g.setColour(cream);
    g.drawLine(cx, cy,
               cx + std::sin(angle) * r * 0.72f,
               cy - std::cos(angle) * r * 0.72f, 2.6f);
    g.setColour(brass);
    g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
}
