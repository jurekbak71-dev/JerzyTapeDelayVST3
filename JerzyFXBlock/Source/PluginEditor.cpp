#include "PluginEditor.h"
namespace{constexpr auto RED=0xffff3030,GREEN=0xff48ef62,YELLOW=0xffffc928; static juce::Colour C(juce::uint32 x){return juce::Colour(x);}}
FXLook::FXLook()
{
    setColour(juce::Slider::textBoxTextColourId,C(0xff9cff85));
    setColour(juce::Slider::textBoxBackgroundColourId,C(0xff071109));
    setColour(juce::Slider::textBoxOutlineColourId,C(0xff315d35));
}
void FXLook::drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,float p,float a0,float a1,juce::Slider&)
{
    auto b=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(5);float d=juce::jmin(b.getWidth(),b.getHeight());
    auto r=juce::Rectangle<float>(d,d).withCentre(b.getCentre()); auto c=r.getCentre();
    g.setColour(C(0xff15181b));g.fillEllipse(r);g.setColour(C(0xff555b60));g.drawEllipse(r,1.2f);
    float a=a0+p*(a1-a0);juce::Path q;q.startNewSubPath(c.x,c.y-d*.08f);q.lineTo(c.x,c.y-d*.35f);q.applyTransform(juce::AffineTransform::rotation(a,c.x,c.y));
    g.setColour(C(RED));g.strokePath(q,juce::PathStrokeType(2.3f));
}
void FXLook::drawToggleButton(juce::Graphics&g,juce::ToggleButton&b,bool,bool)
{
    auto r=b.getLocalBounds().toFloat().reduced(1);auto c=b.findColour(juce::ToggleButton::tickColourId);
    g.setColour(b.getToggleState()?c.withAlpha(.3f):C(0xff111417));g.fillRoundedRectangle(r,4);
    g.setColour(b.getToggleState()?c:C(0xff3b4146));g.drawRoundedRectangle(r,4,1);
    g.setColour(C(0xffe9ecef));g.setFont(juce::FontOptions(11,juce::Font::bold));g.drawFittedText(b.getButtonText(),b.getLocalBounds(),juce::Justification::centred,1);
}
juce::Font FXLook::getLabelFont(juce::Label&l){return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),juce::jmax(9.f,l.getHeight()*.4f),juce::Font::plain));}

JerzyFXBlockAudioProcessorEditor::JerzyFXBlockAudioProcessorEditor(JerzyFXBlockAudioProcessor&p):AudioProcessorEditor(&p),proc(p)
{
    setLookAndFeel(&look);setResizable(true,true);setResizeLimits(900,420,1800,840);getConstrainer()->setFixedAspectRatio(2.142857);setSize(1350,630);
    title.setText("JERZY FX BLOCK",juce::dontSendNotification);title.setColour(juce::Label::textColourId,C(0xffe9ecef));title.setJustificationType(juce::Justification::centredLeft);
    sub.setText("REVERB  >  DELAY  >  CHORUS  >  STEREO  >  ROTARY  >  SHIMMER",juce::dontSendNotification);sub.setColour(juce::Label::textColourId,C(0xff9cff85));sub.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(title);addAndMakeVisible(sub);
    tog(revOn,"REVERB",C(GREEN));tog(delOn,"DELAY",C(YELLOW));tog(choOn,"CHORUS",C(RED));tog(widOn,"STEREO",C(GREEN));tog(rotOn,"ROTARY",C(YELLOW));tog(shOn,"SHIMMER",C(RED));
    knob(revSize,"SIZE");knob(revDamp,"DAMP");knob(revMix,"MIX");
    knob(delTime,"TIME","ms");knob(delFb,"FDBK");knob(delMix,"MIX");
    knob(choRate,"RATE","Hz");knob(choDepth,"DEPTH","ms");knob(choMix,"MIX");
    knob(width,"WIDTH");knob(widMix,"MIX");
    knob(rotRate,"RATE","Hz");knob(rotDepth,"DEPTH");knob(rotMix,"MIX");
    knob(shAmt,"AMOUNT");knob(shMix,"MIX");
    auto&s=proc.apvts;
    auto SAx=[&](const char*id,FXKnob&k){sa.push_back(std::make_unique<SA>(s,id,k));};
    SAx("revSize",revSize);SAx("revDamp",revDamp);SAx("revMix",revMix);SAx("delTime",delTime);SAx("delFb",delFb);SAx("delMix",delMix);
    SAx("choRate",choRate);SAx("choDepth",choDepth);SAx("choMix",choMix);SAx("width",width);SAx("widMix",widMix);
    SAx("rotRate",rotRate);SAx("rotDepth",rotDepth);SAx("rotMix",rotMix);SAx("shAmt",shAmt);SAx("shMix",shMix);
    auto BAx=[&](const char*id,juce::ToggleButton&b){ba.push_back(std::make_unique<BA>(s,id,b));};
    BAx("revOn",revOn);BAx("delOn",delOn);BAx("choOn",choOn);BAx("widOn",widOn);BAx("rotOn",rotOn);BAx("shOn",shOn);
}
JerzyFXBlockAudioProcessorEditor::~JerzyFXBlockAudioProcessorEditor(){setLookAndFeel(nullptr);}
void JerzyFXBlockAudioProcessorEditor::knob(FXKnob&k,const juce::String&n,const juce::String&u)
{
    k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);k.setTextBoxStyle(juce::Slider::TextBoxBelow,true,92,18);
    k.textFromValueFunction=[n,u](double v){return n+" "+juce::String(v,1)+(u.isEmpty()?"":" "+u);}; addAndMakeVisible(k);
}
void JerzyFXBlockAudioProcessorEditor::tog(juce::ToggleButton&b,const juce::String&t,juce::Colour c){b.setButtonText(t);b.setColour(juce::ToggleButton::tickColourId,c);addAndMakeVisible(b);}
void JerzyFXBlockAudioProcessorEditor::place(juce::Component&c,float x,float y,float w,float h){c.setBounds((int)(x*getWidth()/1350.f),(int)(y*getHeight()/630.f),(int)(w*getWidth()/1350.f),(int)(h*getHeight()/630.f));}
void JerzyFXBlockAudioProcessorEditor::paint(juce::Graphics&g)
{
    g.fillAll(C(0xff050607));g.setColour(C(0xff15181b));g.fillRect(0,0,getWidth(),(int)(65*getHeight()/630.f));
    const char* names[]={"REVERB","DELAY","CHORUS","STEREO EXPANDER","ROTARY","SHIMMER"}; juce::Colour leds[]={C(GREEN),C(YELLOW),C(RED),C(GREEN),C(YELLOW),C(RED)};
    for(int i=0;i<6;++i){float x=15+i*220;auto r=juce::Rectangle<float>(x*getWidth()/1350.f,85*getHeight()/630.f,205*getWidth()/1350.f,520*getHeight()/630.f);
        g.setColour(C(0xff0c0e10));g.fillRoundedRectangle(r,6);g.setColour(C(0xff34383c));g.drawRoundedRectangle(r,6,1.2f);
        g.setColour(leds[i]);g.fillEllipse(r.getX()+12,r.getY()+13,8,8);g.setColour(C(0xffe9ecef));g.setFont(juce::FontOptions(13,juce::Font::bold));g.drawText(names[i],r.withTrimmedLeft(26).removeFromTop(32),juce::Justification::centredLeft);}
}
void JerzyFXBlockAudioProcessorEditor::resized()
{
    title.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),25*getWidth()/1350.f,juce::Font::bold)));
    sub.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),10*getWidth()/1350.f,juce::Font::plain)));
    place(title,20,10,300,42);place(sub,650,12,670,38);
    juce::ToggleButton* bs[]={&revOn,&delOn,&choOn,&widOn,&rotOn,&shOn};for(int i=0;i<6;++i)place(*bs[i],35+i*220,120,160,34);
    place(revSize,35,190,150,125);place(revDamp,35,330,150,125);place(revMix,35,470,150,125);
    place(delTime,255,190,150,125);place(delFb,255,330,150,125);place(delMix,255,470,150,125);
    place(choRate,475,190,150,125);place(choDepth,475,330,150,125);place(choMix,475,470,150,125);
    place(width,695,245,150,125);place(widMix,695,420,150,125);
    place(rotRate,915,190,150,125);place(rotDepth,915,330,150,125);place(rotMix,915,470,150,125);
    place(shAmt,1135,245,150,125);place(shMix,1135,420,150,125);
}
