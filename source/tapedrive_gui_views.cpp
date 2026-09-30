#include "tapedrive_gui_views.h"
#include "vstgui/uidescription/uiviewfactory.h"
#include "vstgui/uidescription/uiviewcreator.h"
#include "vstgui/uidescription/iviewcreator.h"
#include "vstgui/lib/cdrawcontext.h"
#include <algorithm>
#include <cmath>

using namespace VSTGUI;

namespace JerzyAudio {

static const CColor kSteelBase(76,79,78,255);
static const CColor kSteelDark(35,36,35,255);
static const CColor kSteelLight(126,129,126,255);
static const CColor kKnobBlack(16,15,14,255);
static const CColor kKnobEdge(112,106,95,255);
static const CColor kCream(236,217,181,255);
static const CColor kAmber(224,148,43,255);
static const CColor kMeterFace(234,195,122,255);
static const CColor kMeterDark(63,41,20,255);
static const CColor kRed(190,48,32,255);

static CPoint rotatedPoint(double cx,double cy,double x,double y,double a)
{
    const double ca=std::cos(a), sa=std::sin(a);
    return {cx + x*ca - y*sa, cy + x*sa + y*ca};
}

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
    const double cx=(r.left+r.right)*0.5;
    const double cy=(r.top+r.bottom)*0.5;
    const double radius=std::min(r.getWidth(),r.getHeight())*0.43;

    c->setDrawMode(kAntiAliasing | kNonIntegralMode);

    // metal mounting washer
    CRect washer(cx-radius,cy-radius,cx+radius,cy+radius);
    c->setFillColor(CColor(44,43,40,255));
    c->setFrameColor(kSteelLight);
    c->setLineWidth(2.0);
    c->drawEllipse(washer,kDrawFilledAndStroked);

    CRect dark(washer);
    dark.inset(radius*0.12,radius*0.12);
    c->setFillColor(kKnobBlack);
    c->setFrameColor(CColor(6,6,6,255));
    c->drawEllipse(dark,kDrawFilledAndStroked);

    // real chicken-head silhouette
    const double v=std::clamp(static_cast<double>(getValueNormalized()),0.0,1.0);
    const double a=(135.0 + 270.0*v) * Constants::pi / 180.0;
    const double L=radius*0.92;
    const double rear=radius*0.32;
    const double half=radius*0.20;

    CDrawContext::PointList body{
        rotatedPoint(cx,cy,-rear,-half,a),
        rotatedPoint(cx,cy, L*0.66,-half*0.70,a),
        rotatedPoint(cx,cy, L,0,a),
        rotatedPoint(cx,cy, L*0.66, half*0.70,a),
        rotatedPoint(cx,cy,-rear, half,a)
    };
    c->setFillColor(CColor(24,22,19,255));
    c->setFrameColor(kCream);
    c->setLineWidth(1.6);
    c->drawPolygon(body,kDrawFilledAndStroked);

    // ivory pointer insert
    CDrawContext::PointList pointer{
        rotatedPoint(cx,cy,L*0.45,-radius*0.045,a),
        rotatedPoint(cx,cy,L*0.86,0,a),
        rotatedPoint(cx,cy,L*0.45, radius*0.045,a)
    };
    c->setFillColor(kCream);
    c->setFrameColor(CColor(108,75,34,255));
    c->setLineWidth(1.0);
    c->drawPolygon(pointer,kDrawFilledAndStroked);

    CRect hub(cx-radius*0.12,cy-radius*0.12,cx+radius*0.12,cy+radius*0.12);
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

    CRect outer(r);
    c->setFillColor(CColor(24,22,19,255));
    c->setFrameColor(CColor(145,129,99,255));
    c->setLineWidth(3.0);
    c->drawRect(outer,kDrawFilledAndStroked);

    CRect face(r); face.inset(8,8);
    c->setFillColor(kMeterFace);
    c->setFrameColor(CColor(92,64,30,255));
    c->setLineWidth(1.5);
    c->drawRect(face,kDrawFilledAndStroked);

    const double cx=(face.left+face.right)*0.5;
    const double baseY=face.bottom-10;
    const double radius=std::min(face.getWidth()*0.43,face.getHeight()*0.92);

    for(int i=0;i<=12;++i){
        const double t=i/12.0;
        const double deg=205.0 + 130.0*t;
        const double a=deg*Constants::pi/180.0;
        const double r1=radius*(i%2?0.77:0.70);
        const double r2=radius*0.89;
        c->setFrameColor(i>=10?kRed:kMeterDark);
        c->setLineWidth(i%2?1.0:1.8);
        c->drawLine({cx+std::cos(a)*r1,baseY+std::sin(a)*r1},
                    {cx+std::cos(a)*r2,baseY+std::sin(a)*r2});
    }

    const double v=std::clamp(static_cast<double>(getValueNormalized()),0.0,1.0);
    const double a=(205.0 + 130.0*v)*Constants::pi/180.0;
    c->setFrameColor(CColor(83,24,14,255));
    c->setLineWidth(2.5);
    c->drawLine({cx,baseY},{cx+std::cos(a)*radius*0.79,baseY+std::sin(a)*radius*0.79});

    CRect pivot(cx-5,baseY-5,cx+5,baseY+5);
    c->setFillColor(CColor(68,42,20,255));
    c->drawEllipse(pivot,kDrawFilled);
    setDirty(false);
}

ToggleSwitch::ToggleSwitch(const CRect& size, IControlListener* listener, int32_t tag)
: CKnob(size,listener,tag,nullptr,nullptr)
{
    setMin(0.f); setMax(1.f); setWheelInc(1.f);
}

void ToggleSwitch::draw(CDrawContext* c)
{
    CRect r(getViewSize());
    const double cx=(r.left+r.right)*0.5;
    c->setDrawMode(kAntiAliasing | kNonIntegralMode);

    CRect plate(r); plate.inset(8,5);
    c->setFillColor(CColor(54,54,51,255));
    c->setFrameColor(kSteelLight);
    c->setLineWidth(1.5);
    c->drawRect(plate,kDrawFilledAndStroked);

    CRect slot(cx-8,r.top+16,cx+8,r.bottom-16);
    c->setFillColor(CColor(8,8,8,255));
    c->setFrameColor(CColor(120,115,104,255));
    c->drawRect(slot,kDrawFilledAndStroked);

    const bool high=getValueNormalized()>=0.5f;
    const double pivotY=r.getCenter().y;
    const double endY=high ? r.top+24 : r.bottom-24;
    c->setFrameColor(CColor(208,207,198,255));
    c->setLineWidth(7);
    c->drawLine({cx,pivotY},{cx,endY});

    CRect cap(cx-11,endY-11,cx+11,endY+11);
    c->setFillColor(kCream);
    c->setFrameColor(CColor(70,66,58,255));
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
    const double cx=(r.left+r.right)*0.5;
    c->setDrawMode(kAntiAliasing | kNonIntegralMode);

    CRect plate(r); plate.inset(8,5);
    c->setFillColor(CColor(54,54,51,255));
    c->setFrameColor(kSteelLight);
    c->setLineWidth(1.5);
    c->drawRect(plate,kDrawFilledAndStroked);

    CRect slot(cx-8,r.top+14,cx+8,r.bottom-14);
    c->setFillColor(CColor(8,8,8,255));
    c->setFrameColor(CColor(120,115,104,255));
    c->drawRect(slot,kDrawFilledAndStroked);

    const int state=std::clamp(static_cast<int>(std::lround(getValue())),0,2);
    double endY=r.getCenter().y;
    if(state==0) endY=r.bottom-23;
    else if(state==2) endY=r.top+23;

    const double pivotY=r.getCenter().y;
    c->setFrameColor(CColor(208,207,198,255));
    c->setLineWidth(7);
    c->drawLine({cx,pivotY},{cx,endY});

    CRect cap(cx-11,endY-11,cx+11,endY+11);
    c->setFillColor(kCream);
    c->setFrameColor(CColor(70,66,58,255));
    c->drawEllipse(cap,kDrawFilledAndStroked);
    setDirty(false);
}

HardwarePanel::HardwarePanel(const CRect& size, IControlListener* listener, int32_t tag)
: CKnob(size,listener,tag,nullptr,nullptr)
{
    setMouseEnabled(false);
}

void HardwarePanel::draw(CDrawContext* c)
{
    CRect r(getViewSize());
    c->setDrawMode(kAntiAliasing | kNonIntegralMode);

    c->setFillColor(kSteelBase);
    c->setFrameColor(CColor(25,25,24,255));
    c->setLineWidth(3);
    c->drawRect(r,kDrawFilledAndStroked);

    // brushed-steel texture
    for(int y=0;y<static_cast<int>(r.getHeight());y+=4){
        const uint8_t v=static_cast<uint8_t>(86 + ((y/4)%3)*5);
        c->setFrameColor(CColor(v,v+1,v,105));
        c->setLineWidth(1);
        c->drawLine({r.left+2,r.top+y},{r.right-2,r.top+y});
    }

    CRect inner(r); inner.inset(12,12);
    c->setFrameColor(CColor(158,154,143,255));
    c->setLineWidth(2);
    c->drawRect(inner,kDrawStroked);

    auto screw=[&](double x,double y){
        CRect s(x-7,y-7,x+7,y+7);
        c->setFillColor(CColor(172,170,161,255));
        c->setFrameColor(CColor(55,55,52,255));
        c->drawEllipse(s,kDrawFilledAndStroked);
        c->setFrameColor(CColor(70,70,66,255));
        c->setLineWidth(1.5);
        c->drawLine({x-4,y+4},{x+4,y-4});
    };
    screw(r.left+24,r.top+24);
    screw(r.right-24,r.top+24);
    screw(r.left+24,r.bottom-24);
    screw(r.right-24,r.bottom-24);

    // engraved section plates
    c->setFillColor(CColor(39,37,34,170));
    c->setFrameColor(CColor(188,135,57,255));
    c->setLineWidth(1.5);
    c->drawRect({35,112,965,302},kDrawFilledAndStroked);
    c->drawRect({35,520,625,666},kDrawFilledAndStroked);
    c->drawRect({642,520,965,666},kDrawFilledAndStroked);

    setDirty(false);
}

namespace {

template <typename T>
class SimpleCreator : public ViewCreatorAdapter {
public:
    SimpleCreator(const char* name,const char* base):name(name),base(base){ UIViewFactory::registerViewCreator(*this); }
    IdStringPtr getViewName() const override { return name; }
    IdStringPtr getBaseViewName() const override { return base; }
    CView* create(const UIAttributes&, const IUIDescription*) const override {
        return new T(CRect(0,0,100,100),nullptr,-1);
    }
private:
    const char* name;
    const char* base;
};

static SimpleCreator<ChickenKnob> gChicken("ChickenKnob","CKnob");
static SimpleCreator<AnalogMeter> gMeter("AnalogMeter","CKnob");
static SimpleCreator<ToggleSwitch> gToggle("ToggleSwitch","CKnob");
static SimpleCreator<ThreeWaySwitch> gThree("ThreeWaySwitch","CKnob");
static SimpleCreator<HardwarePanel> gPanel("HardwarePanel","CKnob");

}

void registerTapeDriveViews() {}

}
