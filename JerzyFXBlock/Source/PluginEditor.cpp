#include "PluginEditor.h"

namespace
{
constexpr auto RED=0xffff3030, GREEN=0xff48ef62, YELLOW=0xffffc928;
static juce::Colour C(juce::uint32 x){return juce::Colour(x);}
static const juce::Colour lcdBg=C(0xffc9d0c6), lcdText=C(0xff111613);
}

FXLook::FXLook()
{
    setColour(juce::Slider::textBoxTextColourId,lcdText);
    setColour(juce::Slider::textBoxBackgroundColourId,lcdBg);
    setColour(juce::Slider::textBoxOutlineColourId,C(0xff596159));
    setColour(juce::ComboBox::textColourId,lcdText);
    setColour(juce::ComboBox::backgroundColourId,lcdBg);
    setColour(juce::ComboBox::outlineColourId,C(0xff596159));
    setColour(juce::PopupMenu::backgroundColourId,C(0xffd5dbd2));
    setColour(juce::PopupMenu::textColourId,lcdText);
}

void FXLook::drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,float p,float a0,float a1,juce::Slider&)
{
    auto b=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(5);
    float d=juce::jmin(b.getWidth(),b.getHeight());
    auto r=juce::Rectangle<float>(d,d).withCentre(b.getCentre()); auto cc=r.getCentre();
    g.setColour(C(0xff15181b));g.fillEllipse(r);g.setColour(C(0xff555b60));g.drawEllipse(r,1.2f);
    float a=a0+p*(a1-a0);juce::Path q;q.startNewSubPath(cc.x,cc.y-d*.08f);q.lineTo(cc.x,cc.y-d*.35f);q.applyTransform(juce::AffineTransform::rotation(a,cc.x,cc.y));
    g.setColour(C(RED));g.strokePath(q,juce::PathStrokeType(2.3f));
}
void FXLook::drawToggleButton(juce::Graphics&g,juce::ToggleButton&b,bool,bool)
{
    auto r=b.getLocalBounds().toFloat().reduced(1);auto col=b.findColour(juce::ToggleButton::tickColourId);
    g.setColour(b.getToggleState()?col.withAlpha(.28f):C(0xff101214));g.fillRoundedRectangle(r,4);
    g.setColour(b.getToggleState()?col:C(0xff3b4146));g.drawRoundedRectangle(r,4,1);
    g.setColour(lcdBg);g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),11.0f,juce::Font::bold)));
    g.drawFittedText(b.getButtonText(),b.getLocalBounds(),juce::Justification::centred,1);
}
void FXLook::drawComboBox(juce::Graphics&g,int w,int h,bool,int,int,int,int,juce::ComboBox&)
{
    auto r=juce::Rectangle<float>(0,0,(float)w,(float)h).reduced(1);
    g.setColour(lcdBg);g.fillRoundedRectangle(r,3);
    g.setColour(C(0xff596159));g.drawRoundedRectangle(r,3,1);
    juce::Path p; float cx=w-11.0f,cy=h*.5f;p.startNewSubPath(cx-3,cy-2);p.lineTo(cx,cy+2);p.lineTo(cx+3,cy-2);
    g.setColour(lcdText);g.strokePath(p,juce::PathStrokeType(1.2f));
}
juce::Font FXLook::getLabelFont(juce::Label&l)
{
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),juce::jmax(9.f,l.getHeight()*.42f),juce::Font::bold));
}
juce::Font FXLook::getComboBoxFont(juce::ComboBox&b)
{
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),juce::jmax(9.f,b.getHeight()*.38f),juce::Font::bold));
}
void FXLook::positionComboBoxText(juce::ComboBox&b,juce::Label&l){l.setBounds(6,1,b.getWidth()-20,b.getHeight()-2);l.setFont(getComboBoxFont(b));}

JerzyFXBlockAudioProcessorEditor::JerzyFXBlockAudioProcessorEditor(JerzyFXBlockAudioProcessor&p):AudioProcessorEditor(&p),proc(p)
{
    setLookAndFeel(&look); setOpaque(true); setResizable(true,true);
    setResizeLimits(900,420,1800,840); getConstrainer()->setFixedAspectRatio(2.142857); setSize(1350,630);
    order=proc.getEffectOrder();

    title.setText("JERZY FX BLOCK",juce::dontSendNotification);
    title.setColour(juce::Label::textColourId,lcdText); title.setColour(juce::Label::backgroundColourId,lcdBg);
    title.setJustificationType(juce::Justification::centredLeft);
    sub.setColour(juce::Label::textColourId,lcdText); sub.setColour(juce::Label::backgroundColourId,lcdBg);
    sub.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(title);addAndMakeVisible(sub); updateOrderCaption();

    tog(revOn,"REVERB",C(GREEN));tog(delOn,"DELAY",C(YELLOW));tog(choOn,"CHORUS",C(RED));
    tog(widOn,"STEREO",C(GREEN));tog(rotOn,"ROTARY",C(YELLOW));tog(shOn,"SHIMMER",C(RED));
    tog(delSync,"SYNC",C(YELLOW));tog(rotSync,"SYNC",C(YELLOW));
    combo(delDivision,{"1/1","1/2","1/4","1/8","1/16","1/32","1/4T","1/8T","1/16T","1/4D","1/8D","1/16D"});
    combo(rotDivision,{"1/1","1/2","1/4","1/8","1/16","1/32","1/4T","1/8T","1/16T","1/4D","1/8D","1/16D"});

    knob(revSize,"SIZE","",0.50);knob(revDamp,"DAMP","",0.50);knob(revMix,"MIX","",0.0);
    knob(delTime,"TIME","ms",1.0);knob(delFb,"FDBK","",0.0);knob(delMix,"MIX","",0.0);
    knob(choRate,"RATE","Hz",0.50);knob(choDepth,"DEPTH","ms",0.0);knob(choMix,"MIX","",0.0);
    knob(width,"WIDTH","",0.0);knob(widMix,"MIX","",0.0);
    knob(rotRate,"RATE","Hz",1.0);knob(rotDepth,"DEPTH","",0.0);knob(rotMix,"MIX","",0.0);
    knob(shAmt,"AMOUNT","",0.0);knob(shMix,"MIX","",0.0);

    auto&s=proc.apvts;
    auto SAx=[&](const char*id,FXKnob&k){sa.push_back(std::make_unique<SA>(s,id,k));};
    SAx("revSize",revSize);SAx("revDamp",revDamp);SAx("revMix",revMix);
    SAx("delTime",delTime);SAx("delFb",delFb);SAx("delMix",delMix);
    SAx("choRate",choRate);SAx("choDepth",choDepth);SAx("choMix",choMix);
    SAx("width",width);SAx("widMix",widMix);
    SAx("rotRate",rotRate);SAx("rotDepth",rotDepth);SAx("rotMix",rotMix);
    SAx("shAmt",shAmt);SAx("shMix",shMix);
    auto BAx=[&](const char*id,juce::ToggleButton&b){ba.push_back(std::make_unique<BA>(s,id,b));};
    BAx("revOn",revOn);BAx("delOn",delOn);BAx("choOn",choOn);BAx("widOn",widOn);BAx("rotOn",rotOn);BAx("shOn",shOn);
    BAx("delSync",delSync);BAx("rotSync",rotSync);
    ca.push_back(std::make_unique<CA>(s,"delDivision",delDivision));
    ca.push_back(std::make_unique<CA>(s,"rotDivision",rotDivision));

    delSync.onClick=[this]{updateSyncControls();};
    rotSync.onClick=[this]{updateSyncControls();};
    delDivision.onChange=[this]{repaint();};
    rotDivision.onChange=[this]{repaint();};
    updateSyncControls();
    startTimerHz(15);
}
JerzyFXBlockAudioProcessorEditor::~JerzyFXBlockAudioProcessorEditor(){stopTimer();setLookAndFeel(nullptr);}

void JerzyFXBlockAudioProcessorEditor::knob(FXKnob&k,const juce::String&n,const juce::String&u,double neutral)
{
    k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.setTextBoxStyle(juce::Slider::TextBoxBelow,true,150,22);
    k.textFromValueFunction=[n,u](double v){return juce::String(v,1)+(u.isEmpty()?"":" "+u);};
    k.setNeutralValue(neutral);
    k.setTooltip(n);
    addAndMakeVisible(k);
}
void JerzyFXBlockAudioProcessorEditor::tog(juce::ToggleButton&b,const juce::String&t,juce::Colour c){b.setButtonText(t);b.setColour(juce::ToggleButton::tickColourId,c);addAndMakeVisible(b);}
void JerzyFXBlockAudioProcessorEditor::combo(juce::ComboBox&b,const juce::StringArray&i){b.addItemList(i,1);addAndMakeVisible(b);}
void JerzyFXBlockAudioProcessorEditor::place(juce::Component&c,float x,float y,float w,float h){c.setBounds((int)(x*getWidth()/1350.f),(int)(y*getHeight()/630.f),(int)(w*getWidth()/1350.f),(int)(h*getHeight()/630.f));}
int JerzyFXBlockAudioProcessorEditor::slotForEffect(int id) const{for(int i=0;i<6;++i)if(order[(size_t)i]==id)return i;return 0;}
void JerzyFXBlockAudioProcessorEditor::updateOrderCaption()
{
    static const char* n[]={"REV","DEL","CHO","WID","ROT","SHM"};
    juce::String s="CHAIN  ";for(int i=0;i<6;++i){if(i)s<<" > ";s<<n[order[(size_t)i]];}sub.setText(s,juce::dontSendNotification);
}
void JerzyFXBlockAudioProcessorEditor::updateSyncControls()
{
    const bool ds=delSync.getToggleState(), rs=rotSync.getToggleState();
    delTime.setEnabled(!ds); delDivision.setEnabled(ds);
    rotRate.setEnabled(!rs); rotDivision.setEnabled(rs);
    resized(); repaint();
}
void JerzyFXBlockAudioProcessorEditor::timerCallback()
{
    const bool ds=delSync.getToggleState(), rs=rotSync.getToggleState();
    if(delDivision.isEnabled()!=ds || delTime.isEnabled()==ds || rotDivision.isEnabled()!=rs || rotRate.isEnabled()==rs)
        updateSyncControls();
}
void JerzyFXBlockAudioProcessorEditor::drawParamLabel(juce::Graphics& g,const juce::String& t,float x,float y,float w) const
{
    auto rr=juce::Rectangle<float>(x*getWidth()/1350.f,y*getHeight()/630.f,w*getWidth()/1350.f,18*getHeight()/630.f);
    g.setColour(lcdBg);g.fillRoundedRectangle(rr,2.5f);
    g.setColour(lcdText);
    const float fs=juce::jlimit(7.0f,11.0f,9.5f*getWidth()/1350.f);
    g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),fs,juce::Font::bold)));
    g.drawFittedText(t,rr.toNearestInt().reduced(2,0),juce::Justification::centred,1,0.80f);
}

void JerzyFXBlockAudioProcessorEditor::mouseDown(const juce::MouseEvent&e)
{
    const float bx=e.position.x*1350.0f/getWidth(), by=e.position.y*630.0f/getHeight();
    if(by>=85.0f&&by<=118.0f&&bx>=15.0f&&bx<=1335.0f)
    {
        int s=(int)((bx-15.0f)/220.0f);
        if(s>=0&&s<6)dragSlot=s;
    }
}
void JerzyFXBlockAudioProcessorEditor::mouseDrag(const juce::MouseEvent&e)
{
    if(dragSlot<0)return;
    const float bx=e.position.x*1350.0f/getWidth();
    int target=juce::jlimit(0,5,(int)((bx-15.0f)/220.0f));
    if(target!=dragSlot)
    {
        const int moving=order[(size_t)dragSlot];
        if(target>dragSlot)for(int i=dragSlot;i<target;++i)order[(size_t)i]=order[(size_t)i+1];
        else for(int i=dragSlot;i>target;--i)order[(size_t)i]=order[(size_t)i-1];
        order[(size_t)target]=moving;dragSlot=target;
        proc.setEffectOrder(order);updateOrderCaption();resized();repaint();
    }
}
void JerzyFXBlockAudioProcessorEditor::mouseUp(const juce::MouseEvent&){dragSlot=-1;}

void JerzyFXBlockAudioProcessorEditor::paint(juce::Graphics&g)
{
    g.fillAll(C(0xff050607));g.setColour(C(0xff15181b));g.fillRect(0,0,getWidth(),(int)(65*getHeight()/630.f));
    static const char* names[]={"REVERB","DELAY","CHORUS","STEREO EXPANDER","ROTARY","SHIMMER"};
    const juce::Colour leds[]={C(GREEN),C(YELLOW),C(RED),C(GREEN),C(YELLOW),C(RED)};
    for(int slot=0;slot<6;++slot)
    {
        const int id=order[(size_t)slot];float x=slotX(slot);
        auto r=juce::Rectangle<float>(x*getWidth()/1350.f,85*getHeight()/630.f,205*getWidth()/1350.f,520*getHeight()/630.f);
        g.setColour(C(0xff0c0e10));g.fillRoundedRectangle(r,6);g.setColour(C(0xff34383c));g.drawRoundedRectangle(r,6,1.2f);
        auto hdr=r.removeFromTop(34*getHeight()/630.f);
        g.setColour(lcdBg);g.fillRoundedRectangle(hdr.reduced(4,3),3);
        g.setColour(leds[id]);g.fillEllipse(hdr.getX()+10,hdr.getCentreY()-4,8,8);
        g.setColour(lcdText);g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),12.0f*getWidth()/1350.f,juce::Font::bold)));
        g.drawText(juce::String(":: ")+names[id]+" ::",hdr.withTrimmedLeft(24),juce::Justification::centredLeft);
    }
    auto bx=[&](int id){return slotX(slotForEffect(id));};
    float x=bx(0);drawParamLabel(g,"SIZE",x+35,302,136);drawParamLabel(g,"DAMPING",x+35,442,136);drawParamLabel(g,"MIX",x+35,582,136);
    x=bx(1);drawParamLabel(g,"TIME",x+35,315,136);drawParamLabel(g,"FEEDBACK",x+35,445,136);drawParamLabel(g,"MIX",x+35,575,136);
    x=bx(2);drawParamLabel(g,"RATE",x+35,302,136);drawParamLabel(g,"DEPTH",x+35,442,136);drawParamLabel(g,"MIX",x+35,582,136);
    x=bx(3);drawParamLabel(g,"WIDTH",x+35,357,136);drawParamLabel(g,"MIX",x+35,532,136);
    x=bx(4);drawParamLabel(g,"RATE",x+35,315,136);drawParamLabel(g,"DEPTH",x+35,445,136);drawParamLabel(g,"MIX",x+35,575,136);
    x=bx(5);drawParamLabel(g,"AMOUNT",x+35,357,136);drawParamLabel(g,"MIX",x+35,532,136);

    g.setColour(lcdBg);
    g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),juce::jlimit(7.0f,10.0f,9.0f*getWidth()/1350.f),juce::Font::bold)));
    g.drawFittedText("DRAG MODULE HEADER TO CHANGE SIGNAL ORDER",
                     juce::Rectangle<int>((int)(20*getWidth()/1350.f),(int)(607*getHeight()/630.f),(int)(600*getWidth()/1350.f),(int)(18*getHeight()/630.f)),
                     juce::Justification::centredLeft,1,0.8f);
}

void JerzyFXBlockAudioProcessorEditor::resized()
{
    const float sc=getWidth()/1350.f;
    title.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),23*sc,juce::Font::bold)));
    sub.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),10*sc,juce::Font::bold)));
    place(title,20,10,300,42);place(sub,520,12,800,38);

    auto base=[&](int id){return slotX(slotForEffect(id));};
    float x=base(0);place(revOn,x+22,126,160,30);place(revSize,x+28,190,150,112);place(revDamp,x+28,330,150,112);place(revMix,x+28,470,150,112);
    x=base(1);place(delOn,x+22,126,160,30);place(delSync,x+18,162,72,28);place(delDivision,x+96,162,92,28);place(delTime,x+28,210,150,105);place(delFb,x+28,340,150,105);place(delMix,x+28,470,150,105);
    x=base(2);place(choOn,x+22,126,160,30);place(choRate,x+28,190,150,112);place(choDepth,x+28,330,150,112);place(choMix,x+28,470,150,112);
    x=base(3);place(widOn,x+22,126,160,30);place(width,x+28,245,150,112);place(widMix,x+28,420,150,112);
    x=base(4);place(rotOn,x+22,126,160,30);place(rotSync,x+18,162,72,28);place(rotDivision,x+96,162,92,28);place(rotRate,x+28,210,150,105);place(rotDepth,x+28,340,150,105);place(rotMix,x+28,470,150,105);
    x=base(5);place(shOn,x+22,126,160,30);place(shAmt,x+28,245,150,112);place(shMix,x+28,420,150,112);

    const bool ds=delSync.getToggleState(), rs=rotSync.getToggleState();
    delTime.setEnabled(!ds);delDivision.setEnabled(ds);
    rotRate.setEnabled(!rs);rotDivision.setEnabled(rs);
}
