#include "PluginEditor.h"

namespace
{
constexpr auto red    = 0xffff3030;
constexpr auto green  = 0xff48ef62;
constexpr auto yellow = 0xffffc928;
constexpr auto panel  = 0xff0c0e10;
constexpr auto edge   = 0xff34383c;
constexpr auto text   = 0xffe9ecef;

static juce::Colour C(juce::uint32 argb) { return juce::Colour(argb); }

static void drawLed(juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col)
{
    g.setColour(col.withAlpha(0.16f));
    g.fillEllipse(c.x-r*2.2f, c.y-r*2.2f, r*4.4f, r*4.4f);
    g.setColour(col);
    g.fillEllipse(c.x-r, c.y-r, r*2.0f, r*2.0f);
    g.setColour(juce::Colours::white.withAlpha(0.45f));
    g.fillEllipse(c.x-r*0.45f, c.y-r*0.55f, r*0.55f, r*0.55f);
}
}

JerzyLookAndFeel::JerzyLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, C(0xff8dff78));
    setColour(juce::Slider::textBoxBackgroundColourId, C(0xff071109));
    setColour(juce::Slider::textBoxOutlineColourId, C(0xff254a29));
    setColour(juce::Label::textColourId, C(0xffa7ff92));
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::textColourId, C(0xff9cff85));
    setColour(juce::ComboBox::backgroundColourId, C(0xff071109));
    setColour(juce::ComboBox::outlineColourId, C(0xff315d35));
    setColour(juce::PopupMenu::backgroundColourId, C(0xff111417));
    setColour(juce::PopupMenu::textColourId, C(text));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, C(0xff30363c));
}

void JerzyLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider&)
{
    auto bounds = juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(5.0f);
    const float d = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto r = juce::Rectangle<float>(d,d).withCentre(bounds.getCentre());
    auto c = r.getCentre();
    float radius = d * 0.45f;

    juce::ColourGradient shadow(C(0xff2d3237), c.x-radius, c.y-radius, C(0xff030405), c.x+radius, c.y+radius, false);
    g.setGradientFill(shadow);
    g.fillEllipse(r);

    g.setColour(C(0xff555b60));
    g.drawEllipse(r, juce::jmax(1.0f, d*0.018f));

    const float angle = startAngle + sliderPos * (endAngle-startAngle);
    juce::Path p;
    p.startNewSubPath(c.x, c.y-radius*0.18f);
    p.lineTo(c.x, c.y-radius*0.78f);
    p.applyTransform(juce::AffineTransform::rotation(angle, c.x, c.y));
    g.setColour(C(red));
    g.strokePath(p, juce::PathStrokeType(juce::jmax(2.0f,d*0.035f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(juce::Colours::white.withAlpha(0.16f));
    auto arcR = r.reduced(d*0.09f);
    juce::Path arc;
    arc.addCentredArc(arcR.getCentreX(), arcR.getCentreY(),
                      arcR.getWidth()*0.5f, arcR.getHeight()*0.5f,
                      0.0f, startAngle, endAngle, true);
    g.strokePath(arc, juce::PathStrokeType(juce::jmax(1.0f,d*0.012f)));
}

void JerzyLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                                           float sliderPos, float minSliderPos, float maxSliderPos,
                                           juce::Slider::SliderStyle style, juce::Slider& slider)
{
    auto r = juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h);
    if (style == juce::Slider::LinearVertical)
    {
        auto track = juce::Rectangle<float>(r.getCentreX()-3.0f, r.getY()+5.0f, 6.0f, r.getHeight()-10.0f);
        g.setColour(C(0xff050607)); g.fillRoundedRectangle(track, 3.0f);
        g.setColour(C(0xff3a3f44)); g.drawRoundedRectangle(track, 3.0f, 1.0f);

        auto fill = juce::Rectangle<float>(track.getX(), sliderPos, track.getWidth(), track.getBottom()-sliderPos);
        g.setColour(C(0xff697077)); g.fillRoundedRectangle(fill, 3.0f);

        const float knobH = juce::jmax(8.0f, r.getWidth()*0.20f);
        auto knob = juce::Rectangle<float>(r.getX()+4.0f, sliderPos-knobH*0.5f, r.getWidth()-8.0f, knobH);
        g.setColour(C(0xff171a1d)); g.fillRoundedRectangle(knob, 2.0f);
        g.setColour(C(red)); g.drawRoundedRectangle(knob, 2.0f, 1.5f);
        g.setColour(C(red)); g.fillRect(knob.reduced(3.0f).withHeight(2.0f).withCentre(knob.getCentre()));
        return;
    }
    juce::LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,sliderPos,minSliderPos,maxSliderPos,style,slider);
}

void JerzyLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    auto on = b.findColour(juce::ToggleButton::tickColourId);
    auto base = b.getToggleState() ? on : C(0xff15181b);
    if (over) base = base.brighter(0.12f);
    if (down) base = base.darker(0.15f);

    g.setColour(base.withAlpha(b.getToggleState() ? 0.30f : 1.0f));
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(b.getToggleState() ? on : C(0xff3f4449));
    g.drawRoundedRectangle(r, 4.0f, 1.0f);
    if (b.getToggleState())
    {
        g.setColour(on.withAlpha(0.15f));
        g.fillRoundedRectangle(r.expanded(2.0f), 5.0f);
    }
    g.setColour(C(text));
    g.setFont(juce::FontOptions(juce::jmax(10.0f, r.getHeight()*0.30f), juce::Font::bold));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(4), juce::Justification::centred, 1);
}

void JerzyLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float>(0,0,(float)w,(float)h).reduced(1.0f);
    g.setColour(C(0xff111417)); g.fillRoundedRectangle(r, 4.0f);
    g.setColour(C(0xff40464b)); g.drawRoundedRectangle(r, 4.0f, 1.0f);
    juce::Path a;
    const float cx=w-13.0f, cy=h*0.5f;
    a.startNewSubPath(cx-4,cy-2); a.lineTo(cx,cy+2); a.lineTo(cx+4,cy-2);
    g.setColour(C(0xffbfc5ca)); g.strokePath(a, juce::PathStrokeType(1.5f));
}
juce::Font JerzyLookAndFeel::getComboBoxFont(juce::ComboBox& b)
{
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                        juce::jmax(10.0f, b.getHeight()*0.30f),
                                        juce::Font::bold));
}
juce::Font JerzyLookAndFeel::getLabelFont(juce::Label& l)
{
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                        juce::jmax(9.0f, l.getHeight()*0.42f),
                                        juce::Font::plain));
}
void JerzyLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(8,1,box.getWidth()-24,box.getHeight()-2);
    label.setFont(getComboBoxFont(box));
}

void JerzyMonoAnalogAudioProcessorEditor::OutputMeter::paint(juce::Graphics& g)
{
    auto r=getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(C(0xff070809)); g.fillRoundedRectangle(r,3.0f);
    const int n=14;
    const float gap=2.0f, seg=(r.getWidth()-gap*(n-1))/n;
    for(int i=0;i<n;++i)
    {
        auto rr=juce::Rectangle<float>(r.getX()+i*(seg+gap),r.getY()+2,seg,r.getHeight()-4);
        const float t=(i+1)/(float)n;
        juce::Colour col=t<0.58f?C(green):(t<0.82f?C(yellow):C(red));
        g.setColour(t<=level?col:col.withAlpha(0.12f));
        g.fillRoundedRectangle(rr,1.0f);
    }
}

JerzyMonoAnalogAudioProcessorEditor::JerzyMonoAnalogAudioProcessorEditor(JerzyMonoAnalogAudioProcessor& p)
: AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&look);
    setOpaque(true);
    setResizable(true,true);
    setResizeLimits(900,450,1920,960);
    getConstrainer()->setFixedAspectRatio(2.0);
    setSize(1440,720);

    title.setText("JERZY", juce::dontSendNotification);
    title.setColour(juce::Label::textColourId, C(text)); title.setJustificationType(juce::Justification::centredLeft);
    subtitle.setText("MONO ANALOG", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, C(0xffc5c9cc)); subtitle.setJustificationType(juce::Justification::centredLeft);
    preset.setText("01  CLASSIC MONO", juce::dontSendNotification);
    preset.setColour(juce::Label::textColourId, C(0xff9cff85)); preset.setJustificationType(juce::Justification::centred);
    preset.setColour(juce::Label::backgroundColourId, C(0xff071109));
    scaleLabel.setText("VECTOR GUI", juce::dontSendNotification);
    scaleLabel.setColour(juce::Label::textColourId, C(0xff9aa0a5)); scaleLabel.setJustificationType(juce::Justification::centredRight);
    for(auto* l:{&title,&subtitle,&preset,&scaleLabel}) addAndMakeVisible(*l);

    setupCombo(osc1Wave,{"SINE","TRI","SAW","SQUARE"}); setupCombo(osc1Oct,{"16'","8'","4'","2'","1'"});
    setupCombo(osc2Wave,{"SINE","TRI","SAW","SQUARE"}); setupCombo(osc2Oct,{"16'","8'","4'","2'","1'"});
    setupCombo(subWave,{"SINE","SQUARE"}); setupCombo(lfoWave,{"SINE","TRI","SAW","SQUARE","S&H"});
    setupCombo(lfoDivision,{"1/1","1/2","1/4","1/8","1/16","1/32","1/4T","1/8T","1/16T","1/4D","1/8D","1/16D"});
    setupCombo(glideMode,{"ALWAYS","LEGATO"}); setupCombo(priority,{"LAST","LOW","HIGH"});

    setupKnob(osc1Level,"OSC1", ""); setupKnob(pulseWidth,"PW", "");
    setupKnob(osc2Level,"OSC2", ""); setupKnob(detune,"DET", "ct");
    setupKnob(subLevel,"SUB", ""); setupKnob(noiseLevel,"NOISE", "");
    setupKnob(mixDrive,"MIX", ""); setupKnob(drift,"DRIFT", "ct");
    setupKnob(cutoff,"CUTOFF", "Hz"); setupKnob(resonance,"RES", "");
    setupKnob(filterDrive,"F.DRV", ""); setupKnob(filterEnv,"ENV", "oct"); setupKnob(keyTrack,"KEY", "");
    setupKnob(lfoRate,"RATE", "Hz"); setupKnob(lfoPitch,"PITCH", "ct");
    setupKnob(lfoFilter,"FILTER", "oct"); setupKnob(lfoPWM,"PWM", "");
    setupKnob(lfoAmp,"AMP", ""); setupKnob(lfoFade,"FADE", "s");
    setupKnob(glide,"GLIDE", "s"); setupKnob(outDrive,"OUT", ""); setupKnob(master,"MASTER", "");

    setupEnvSlider(aA,"A","s"); setupEnvSlider(aD,"D","s"); setupEnvSlider(aS,"S",""); setupEnvSlider(aR,"R","s");
    setupEnvSlider(fA,"A","s"); setupEnvSlider(fD,"D","s"); setupEnvSlider(fS,"S",""); setupEnvSlider(fR,"R","s");

    setupToggle(legato,"LEGATO",C(green)); setupToggle(retrigger,"RETRIGGER",C(red));
    setupToggle(lfoSync,"SYNC",C(yellow));
    addAndMakeVisible(outputMeter);

    addSection("OSC 1",C(green), 15,75,260,215);
    addSection("OSC 2",C(yellow),280,75,285,215);
    addSection("SUB / NOISE",C(red),570,75,220,215);
    addSection("MIXER / DRIVE",C(red),795,75,190,215);
    addSection("FILTER",C(green),990,75,435,215);
    addSection("AMP ENV",C(green),15,300,520,205);
    addSection("FILTER ENV",C(yellow),540,300,520,205);
    addSection("LFO",C(red),1065,300,360,205);
    addSection("GLIDE / PLAY MODE",C(yellow),15,515,900,185);
    addSection("OUTPUT",C(green),920,515,505,185);

    auto& s=proc.apvts;
    osc1WaveA=std::make_unique<ComboAttachment>(s,"osc1Wave",osc1Wave); osc1OctA=std::make_unique<ComboAttachment>(s,"osc1Oct",osc1Oct);
    osc2WaveA=std::make_unique<ComboAttachment>(s,"osc2Wave",osc2Wave); osc2OctA=std::make_unique<ComboAttachment>(s,"osc2Oct",osc2Oct);
    subWaveA=std::make_unique<ComboAttachment>(s,"subWave",subWave); lfoWaveA=std::make_unique<ComboAttachment>(s,"lfoWave",lfoWave);
    lfoDivisionA=std::make_unique<ComboAttachment>(s,"lfoDivision",lfoDivision);
    glideModeA=std::make_unique<ComboAttachment>(s,"glideMode",glideMode); priorityA=std::make_unique<ComboAttachment>(s,"priority",priority);

    osc1LevelA=std::make_unique<SliderAttachment>(s,"osc1Level",osc1Level); pulseWidthA=std::make_unique<SliderAttachment>(s,"pw",pulseWidth);
    osc2LevelA=std::make_unique<SliderAttachment>(s,"osc2Level",osc2Level); detuneA=std::make_unique<SliderAttachment>(s,"detune",detune);
    subLevelA=std::make_unique<SliderAttachment>(s,"subLevel",subLevel); noiseLevelA=std::make_unique<SliderAttachment>(s,"noiseLevel",noiseLevel);
    mixDriveA=std::make_unique<SliderAttachment>(s,"mixDrive",mixDrive); driftA=std::make_unique<SliderAttachment>(s,"drift",drift);
    cutoffA=std::make_unique<SliderAttachment>(s,"cutoff",cutoff); resonanceA=std::make_unique<SliderAttachment>(s,"resonance",resonance);
    filterDriveA=std::make_unique<SliderAttachment>(s,"filterDrive",filterDrive); filterEnvA=std::make_unique<SliderAttachment>(s,"filterEnv",filterEnv);
    keyTrackA=std::make_unique<SliderAttachment>(s,"keyTrack",keyTrack);
    aAA=std::make_unique<SliderAttachment>(s,"aA",aA); aDA=std::make_unique<SliderAttachment>(s,"aD",aD); aSA=std::make_unique<SliderAttachment>(s,"aS",aS); aRA=std::make_unique<SliderAttachment>(s,"aR",aR);
    fAA=std::make_unique<SliderAttachment>(s,"fA",fA); fDA=std::make_unique<SliderAttachment>(s,"fD",fD); fSA=std::make_unique<SliderAttachment>(s,"fS",fS); fRA=std::make_unique<SliderAttachment>(s,"fR",fR);
    lfoRateA=std::make_unique<SliderAttachment>(s,"lfoRate",lfoRate); lfoPitchA=std::make_unique<SliderAttachment>(s,"lfoPitch",lfoPitch);
    lfoFilterA=std::make_unique<SliderAttachment>(s,"lfoFilter",lfoFilter); lfoPWMA=std::make_unique<SliderAttachment>(s,"lfoPWM",lfoPWM);
    lfoAmpA=std::make_unique<SliderAttachment>(s,"lfoAmp",lfoAmp); lfoFadeA=std::make_unique<SliderAttachment>(s,"lfoFade",lfoFade);
    glideA=std::make_unique<SliderAttachment>(s,"glide",glide); outDriveA=std::make_unique<SliderAttachment>(s,"outDrive",outDrive); masterA=std::make_unique<SliderAttachment>(s,"master",master);
    legatoA=std::make_unique<ButtonAttachment>(s,"legato",legato); retriggerA=std::make_unique<ButtonAttachment>(s,"retrigger",retrigger);
    lfoSyncA=std::make_unique<ButtonAttachment>(s,"lfoSync",lfoSync);

    startTimerHz(30);
}

JerzyMonoAnalogAudioProcessorEditor::~JerzyMonoAnalogAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void JerzyMonoAnalogAudioProcessorEditor::setupKnob(ResetSlider& k, const juce::String& name, const juce::String& unit)
{
    k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.setTextBoxStyle(juce::Slider::TextBoxBelow,true,92,17);
    k.textFromValueFunction = [name,unit](double v)
    {
        return name + " " + juce::String(v,1) + (unit.isEmpty() ? "" : " " + unit);
    };
    k.setDoubleClickReturnValue(true,0.0);
    addAndMakeVisible(k);
}
void JerzyMonoAnalogAudioProcessorEditor::setupEnvSlider(ResetSlider& k, const juce::String& name, const juce::String& unit)
{
    k.setSliderStyle(juce::Slider::LinearVertical);
    k.setTextBoxStyle(juce::Slider::TextBoxBelow,true,66,17);
    k.textFromValueFunction = [name,unit](double v)
    {
        return name + " " + juce::String(v,1) + (unit.isEmpty() ? "" : " " + unit);
    };
    addAndMakeVisible(k);
}
void JerzyMonoAnalogAudioProcessorEditor::setupCombo(juce::ComboBox& b, const juce::StringArray& items)
{
    b.addItemList(items,1); addAndMakeVisible(b);
}
void JerzyMonoAnalogAudioProcessorEditor::setupToggle(juce::ToggleButton& b, const juce::String& t, juce::Colour c)
{
    b.setButtonText(t); b.setColour(juce::ToggleButton::tickColourId,c); addAndMakeVisible(b);
}
void JerzyMonoAnalogAudioProcessorEditor::addSection(const juce::String& t, juce::Colour c, float x,float y,float w,float h)
{
    sections.push_back({t,c,{x/1440.0f,y/720.0f,w/1440.0f,h/720.0f}});
}
void JerzyMonoAnalogAudioProcessorEditor::place(juce::Component& c,float x,float y,float w,float h)
{
    c.setBounds(juce::roundToInt(x*sx()),juce::roundToInt(y*sy()),juce::roundToInt(w*sx()),juce::roundToInt(h*sy()));
}

void JerzyMonoAnalogAudioProcessorEditor::drawCtlLabel(juce::Graphics& g, const juce::String& t, float x, float y, float w) const
{
    g.setColour(C(0xffcbd0d4));
    g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                        juce::jmax(8.0f,10.5f*s()),juce::Font::bold)));
    g.drawFittedText(t,
                     juce::Rectangle<int>(juce::roundToInt(x*sx()),juce::roundToInt(y*sy()),
                                          juce::roundToInt(w*sx()),juce::roundToInt(15*sy())),
                     juce::Justification::centred,1);
}

void JerzyMonoAnalogAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ColourGradient bg(C(0xff090b0d),0,0,C(0xff020304),0,(float)getHeight(),false);
    g.setGradientFill(bg); g.fillAll();

    g.setColour(C(0xff171a1d));
    g.fillRect(0,0,getWidth(),juce::roundToInt(65*sy()));
    g.setColour(C(0xff454a4f)); g.drawHorizontalLine(juce::roundToInt(64*sy()),0,(float)getWidth());

    for(const auto& sec:sections) drawSection(g,sec);

    drawEnvelope(g,{35*sx(),325*sy(),480*sx(),45*sy()},false);
    drawEnvelope(g,{560*sx(),325*sy(),480*sx(),45*sy()},true);

    // Czytelne podpisy wszystkich kontrolerow
    drawCtlLabel(g,"WAVE",30,92,115); drawCtlLabel(g,"OCTAVE",150,92,105);
    drawCtlLabel(g,"LEVEL",35,265,95); drawCtlLabel(g,"PULSE WIDTH",145,265,95);
    drawCtlLabel(g,"WAVE",295,92,115); drawCtlLabel(g,"OCTAVE",415,92,105);
    drawCtlLabel(g,"LEVEL",300,265,95); drawCtlLabel(g,"DETUNE",420,265,95);
    drawCtlLabel(g,"SUB WAVE",590,92,180); drawCtlLabel(g,"SUB LEVEL",590,265,85); drawCtlLabel(g,"NOISE",680,265,85);
    drawCtlLabel(g,"MIX DRIVE",815,265,78); drawCtlLabel(g,"ANALOG DRIFT",895,265,78);
    drawCtlLabel(g,"CUTOFF",1010,265,150); drawCtlLabel(g,"RESONANCE",1165,170,78); drawCtlLabel(g,"FILTER DRIVE",1245,170,78);
    drawCtlLabel(g,"ENV AMOUNT",1165,265,78); drawCtlLabel(g,"KEY TRACK",1245,265,78);

    drawCtlLabel(g,"ATTACK",40,475,105); drawCtlLabel(g,"DECAY",160,475,105); drawCtlLabel(g,"SUSTAIN",280,475,105); drawCtlLabel(g,"RELEASE",400,475,105);
    drawCtlLabel(g,"ATTACK",565,475,105); drawCtlLabel(g,"DECAY",685,475,105); drawCtlLabel(g,"SUSTAIN",805,475,105); drawCtlLabel(g,"RELEASE",925,475,105);

    drawCtlLabel(g,"WAVE",1075,318,100); drawCtlLabel(g,"DIVISION",1180,318,105); drawCtlLabel(g,"HOST SYNC",1290,318,115);
    drawCtlLabel(g,"RATE",1070,477,55); drawCtlLabel(g,"PITCH",1128,477,55); drawCtlLabel(g,"FILTER",1186,477,55);
    drawCtlLabel(g,"PWM",1244,477,55); drawCtlLabel(g,"AMP",1302,477,55); drawCtlLabel(g,"FADE IN",1360,477,55);

    drawCtlLabel(g,"GLIDE TIME",40,675,105); drawCtlLabel(g,"GLIDE MODE",160,565,145); drawCtlLabel(g,"NOTE PRIORITY",325,565,145);
    drawCtlLabel(g,"OUTPUT METER",950,548,180); drawCtlLabel(g,"OUTPUT DRIVE",1160,675,100); drawCtlLabel(g,"MASTER",1290,675,110);
}

void JerzyMonoAnalogAudioProcessorEditor::drawSection(juce::Graphics& g,const Section& sec) const
{
    auto r=juce::Rectangle<float>(sec.norm.getX()*getWidth(),sec.norm.getY()*getHeight(),sec.norm.getWidth()*getWidth(),sec.norm.getHeight()*getHeight());
    g.setColour(C(panel)); g.fillRoundedRectangle(r,5.0f*s());
    g.setColour(C(edge)); g.drawRoundedRectangle(r,5.0f*s(),juce::jmax(1.0f,1.2f*s()));
    auto fs=juce::jmax(10.0f,15.0f*s());
    g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),fs,juce::Font::bold)));
    g.setColour(C(text));
    g.drawText(sec.title,r.withTrimmedLeft(28*s()).removeFromTop(30*s()),juce::Justification::centredLeft);
    drawLed(g,{r.getX()+14*s(),r.getY()+15*s()},4*s(),sec.led);
}

void JerzyMonoAnalogAudioProcessorEditor::drawEnvelope(juce::Graphics& g,juce::Rectangle<float> r,bool filter) const
{
    juce::Path p;
    p.startNewSubPath(r.getX(),r.getBottom());
    p.lineTo(r.getX()+r.getWidth()*0.18f,r.getY()+4*s());
    p.lineTo(r.getX()+r.getWidth()*0.38f,r.getY()+r.getHeight()*0.35f);
    p.lineTo(r.getX()+r.getWidth()*0.76f,r.getY()+r.getHeight()*0.35f);
    p.lineTo(r.getRight(),r.getBottom());
    g.setColour((filter?C(yellow):C(green)).withAlpha(0.85f));
    g.strokePath(p,juce::PathStrokeType(juce::jmax(1.0f,1.5f*s())));
}

void JerzyMonoAnalogAudioProcessorEditor::resized()
{
    title.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),28*s(),juce::Font::bold)));
    subtitle.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),15*s(),juce::Font::plain)));
    preset.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),13*s(),juce::Font::bold)));
    scaleLabel.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),11*s(),juce::Font::plain)));
    place(title,25,10,160,42); place(subtitle,180,17,190,30); place(preset,590,16,260,34); place(scaleLabel,1190,18,210,30);

    // OSC1
    place(osc1Wave,30,108,115,30); place(osc1Oct,150,108,105,30);
    place(osc1Level,35,155,95,112); place(pulseWidth,145,155,95,112);
    // OSC2
    place(osc2Wave,295,108,115,30); place(osc2Oct,415,108,105,30);
    place(osc2Level,300,155,95,112); place(detune,420,155,95,112);
    // SUB
    place(subWave,590,108,180,30); place(subLevel,590,155,85,112); place(noiseLevel,680,155,85,112);
    // MIXER
    place(mixDrive,820,145,70,125); place(drift,900,145,70,125);
    // FILTER
    place(cutoff,1010,112,150,160); place(resonance,1165,115,78,120); place(filterDrive,1245,115,78,120);
    place(filterEnv,1165,190,78,90); place(keyTrack,1245,190,78,90);

    // AMP ENV - pionowe suwaki ADSR
    place(aA,55,375,70,95); place(aD,175,375,70,95); place(aS,295,375,70,95); place(aR,415,375,70,95);
    // FILTER ENV - pionowe suwaki ADSR
    place(fA,580,375,70,95); place(fD,700,375,70,95); place(fS,820,375,70,95); place(fR,940,375,70,95);
    // LFO - 7 parametrow: waveform + 6 modulacji
    place(lfoWave,1075,335,100,28); place(lfoDivision,1180,335,105,28); place(lfoSync,1290,335,115,28);
    place(lfoRate,1070,380,55,92); place(lfoPitch,1128,380,55,92); place(lfoFilter,1186,380,55,92);
    place(lfoPWM,1244,380,55,92); place(lfoAmp,1302,380,55,92); place(lfoFade,1360,380,55,92);

    // PLAY
    place(glide,40,558,105,120); place(glideMode,160,585,145,34); place(priority,325,585,145,34);
    place(legato,500,585,135,34); place(retrigger,650,585,145,34);
    // OUTPUT
    place(outputMeter,950,570,180,28); place(outDrive,1160,550,100,125); place(master,1290,540,110,140);
}

void JerzyMonoAnalogAudioProcessorEditor::timerCallback()
{
    outputMeter.setLevel(proc.getOutputMeter());
    const bool sync = lfoSync.getToggleState();
    lfoRate.setEnabled(!sync);
    lfoDivision.setEnabled(sync);
}
