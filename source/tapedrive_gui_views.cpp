#include "tapedrive_gui_views.h"
#include "vstgui/uidescription/uiviewfactory.h"
#include "vstgui/uidescription/uiviewcreator.h"
#include "vstgui/uidescription/iviewcreator.h"
#include "vstgui/lib/cdrawcontext.h"
#include <cmath>

using namespace VSTGUI;

namespace JerzyAudio {

static const CColor kSteelDark(25,23,21,255);
static const CColor kSteelMid(48,44,39,255);
static const CColor kKnobBlack(18,17,16,255);
static const CColor kKnobEdge(92,84,73,255);
static const CColor kCream(238,218,184,255);
static const CColor kAmber(239,160,54,255);
static const CColor kMeterFace(232,190,113,255);
static const CColor kMeterDark(66,43,20,255);
static const CColor kRed(206,58,37,255);

ChickenKnob::ChickenKnob(const CRect& size, IControlListener* listener, int32_t tag)
: CKnob(size, listener, tag, nullptr, nullptr)
{
    setStartAngle(static_cast<float>(Constants::pi * 0.75));
    setRangeAngle(static_cast<float>(Constants::pi * 1.5));
    setWheelInc(0.01f);
}

void ChickenKnob::draw(CDrawContext* c)
{
    CRect r(getViewSize());
    const auto cx=(r.left+r.right)*0.5;
    const auto cy=(r.top+r.bottom)*0.5;
    const auto radius=std::min(r.getWidth(),r.getHeight())*0.46;

    c->setDrawMode(kAntiAliasing | kNonIntegralMode);

    CRect shadow(cx-radius+3,cy-radius+5,cx+radius+3,cy+radius+5);
    c->setFillColor(CColor(0,0,0,120));
    c->setFrameColor(CColor(0,0,0,120));
    c->drawEllipse(shadow,kDrawFilled);

    CRect body(cx-radius,cy-radius,cx+radius,cy+radius);
    c->setFillColor(kKnobBlack);
    c->setFrameColor(kKnobEdge);
    c->setLineWidth(2.0);
    c->drawEllipse(body,kDrawFilledAndStroked);

    CRect inner(body);
    inner.inset(radius*0.18,radius*0.18);
    c->setFillColor(kSteelDark);
    c->setFrameColor(CColor(15,14,13,255));
    c->setLineWidth(1.0);
    c->drawEllipse(inner,kDrawFilledAndStroked);

    // chicken-head pointer: broad triangular beak
    const double v=getValueNormalized();
    const double a=(135.0 + 270.0*v) * Constants::pi / 180.0;
    const CPoint tip(cx+std::cos(a)*radius*0.88, cy+std::sin(a)*radius*0.88);
    const double side=0.22;
    const CPoint p1(cx+std::cos(a+side)*radius*0.26, cy+std::sin(a+side)*radius*0.26);
    const CPoint p2(cx+std::cos(a-side)*radius*0.26, cy+std::sin(a-side)*radius*0.26);
    CDrawContext::PointList poly{p1,tip,p2};
    c->setFillColor(kCream);
    c->setFrameColor(CColor(85,54,21,255));
    c->setLineWidth(1.2);
    c->drawPolygon(poly,kDrawFilledAndStroked);

    // brass hub
    CRect hub(cx-radius*0.13,cy-radius*0.13,cx+radius*0.13,cy+radius*0.13);
    c->setFillColor(kAmber);
    c->setFrameColor(kCream);
    c->setLineWidth(1.0);
    c->drawEllipse(hub,kDrawFilledAndStroked);

    setDirty(false);
}

AnalogMeter::AnalogMeter(const CRect& size, IControlListener* listener, int32_t tag)
: CKnob(size, listener, tag, nullptr, nullptr)
{
    setMin(0.f); setMax(1.f); setValue(0.f);
}

void AnalogMeter::draw(CDrawContext* c)
{
    CRect r(getViewSize());
    c->setDrawMode(kAntiAliasing | kNonIntegralMode);

    c->setFillColor(CColor(14,12,10,255));
    c->setFrameColor(kAmber);
    c->setLineWidth(2.0);
    c->drawRect(r,kDrawFilledAndStroked);

    CRect face(r);
    face.inset(8,8);
    c->setFillColor(kMeterFace);
    c->setFrameColor(CColor(120,75,25,255));
    c->setLineWidth(1.2);
    c->drawRect(face,kDrawFilledAndStroked);

    const auto cx=(face.left+face.right)*0.5;
    const auto baseY=face.bottom-12;
    const auto radius=std::min(face.getWidth()*0.42,face.getHeight()*0.88);

    // scale/ticks
    for(int i=0;i<=10;++i){
        const double t=i/10.0;
        const double deg=210.0 + 120.0*t;
        const double a=deg*Constants::pi/180.0;
        const double rr1=radius*0.74;
        const double rr2=radius*0.88;
        CPoint p1(cx+std::cos(a)*rr1,baseY+std::sin(a)*rr1);
        CPoint p2(cx+std::cos(a)*rr2,baseY+std::sin(a)*rr2);
        c->setFrameColor(i>=8?kRed:kMeterDark);
        c->setLineWidth(i%2?1.0:1.6);
        c->drawLine(p1,p2);
    }

    const double v=std::clamp(static_cast<double>(getValueNormalized()),0.0,1.0);
    const double deg=210.0 + 120.0*v;
    const double a=deg*Constants::pi/180.0;
    CPoint needle(cx+std::cos(a)*radius*0.78,baseY+std::sin(a)*radius*0.78);
    c->setFrameColor(CColor(76,22,12,255));
    c->setLineWidth(2.4);
    c->drawLine(CPoint(cx,baseY),needle);

    CRect pivot(cx-5,baseY-5,cx+5,baseY+5);
    c->setFillColor(CColor(64,38,18,255));
    c->drawEllipse(pivot,kDrawFilled);

    setDirty(false);
}

namespace {

class ChickenKnobCreator : public ViewCreatorAdapter {
public:
    ChickenKnobCreator(){ UIViewFactory::registerViewCreator(*this); }
    IdStringPtr getViewName() const override { return "ChickenKnob"; }
    IdStringPtr getBaseViewName() const override { return "CKnob"; }
    CView* create(const UIAttributes&, const IUIDescription*) const override {
        return new ChickenKnob(CRect(0,0,100,100),nullptr,-1);
    }
};

class AnalogMeterCreator : public ViewCreatorAdapter {
public:
    AnalogMeterCreator(){ UIViewFactory::registerViewCreator(*this); }
    IdStringPtr getViewName() const override { return "AnalogMeter"; }
    IdStringPtr getBaseViewName() const override { return "CKnob"; }
    CView* create(const UIAttributes&, const IUIDescription*) const override {
        return new AnalogMeter(CRect(0,0,220,110),nullptr,-1);
    }
};

static ChickenKnobCreator gChickenKnobCreator;
class ToggleSwitchCreator : public ViewCreatorAdapter {
public:
    ToggleSwitchCreator(){ UIViewFactory::registerViewCreator(*this); }
    IdStringPtr getViewName() const override { return "ToggleSwitch"; }
    IdStringPtr getBaseViewName() const override { return "CKnob"; }
    CView* create(const UIAttributes&, const IUIDescription*) const override {
        return new ToggleSwitch(CRect(0,0,70,100),nullptr,-1);
    }
};

class ThreeWaySwitchCreator : public ViewCreatorAdapter {
public:
    ThreeWaySwitchCreator(){ UIViewFactory::registerViewCreator(*this); }
    IdStringPtr getViewName() const override { return "ThreeWaySwitch"; }
    IdStringPtr getBaseViewName() const override { return "CKnob"; }
    CView* create(const UIAttributes&, const IUIDescription*) const override {
        return new ThreeWaySwitch(CRect(0,0,70,100),nullptr,-1);
    }
};

static AnalogMeterCreator gAnalogMeterCreator;
static ToggleSwitchCreator gToggleSwitchCreator;
static ThreeWaySwitchCreator gThreeWaySwitchCreator;

}

ToggleSwitch::ToggleSwitch(const CRect& size, IControlListener* listener, int32_t tag)
: CKnob(size,listener,tag,nullptr,nullptr)
{
    setMin(0.f); setMax(1.f); setWheelInc(1.f);
}

void ToggleSwitch::draw(CDrawContext* c)
{
    CRect r(getViewSize());
    const auto cx=(r.left+r.right)*0.5;
    c->setDrawMode(kAntiAliasing | kNonIntegralMode);
    CRect slot(cx-10,r.top+12,cx+10,r.bottom-12);
    c->setFillColor(CColor(12,11,10,255));
    c->setFrameColor(kKnobEdge);
    c->setLineWidth(2);
    c->drawRect(slot,kDrawFilledAndStroked);

    const bool high=getValueNormalized()>=0.5f;
    const double y=high ? r.top+28 : r.bottom-28;
    c->setFrameColor(CColor(190,185,170,255));
    c->setLineWidth(7);
    c->drawLine(CPoint(cx,y),CPoint(cx,high?y+25:y-25));

    CRect cap(cx-11,(high?y+20:y-30),cx+11,(high?y+42:y-8));
    c->setFillColor(kCream);
    c->setFrameColor(CColor(80,74,65,255));
    c->setLineWidth(1.5);
    c->drawEllipse(cap,kDrawFilledAndStroked);
    setDirty(false);
}

ThreeWaySwitch::ThreeWaySwitch(const CRect& size, IControlListener* listener, int32_t tag)
: CKnob(size,listener,tag,nullptr,nullptr)
{
    setMin(0.f); setMax(2.f); setWheelInc(1.f);
}

void ThreeWaySwitch::draw(CDrawContext* c)
{
    CRect r(getViewSize());
    const auto cx=(r.left+r.right)*0.5;
    c->setDrawMode(kAntiAliasing | kNonIntegralMode);
    CRect slot(cx-10,r.top+10,cx+10,r.bottom-10);
    c->setFillColor(CColor(12,11,10,255));
    c->setFrameColor(kKnobEdge);
    c->setLineWidth(2);
    c->drawRect(slot,kDrawFilledAndStroked);

    const int state=std::clamp((int)std::lround(getValue()),0,2);
    double y=r.getCenter().y;
    if(state==0) y=r.bottom-27;
    else if(state==2) y=r.top+27;

    c->setFrameColor(CColor(190,185,170,255));
    c->setLineWidth(7);
    c->drawLine(CPoint(cx,y),CPoint(cx,y-24));

    CRect cap(cx-11,y-35,cx+11,y-13);
    c->setFillColor(kCream);
    c->setFrameColor(CColor(80,74,65,255));
    c->setLineWidth(1.5);
    c->drawEllipse(cap,kDrawFilledAndStroked);
    setDirty(false);
}

void registerTapeDriveViews() {}

}
