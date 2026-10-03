#include "tapedrive_gui_views.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicstransform.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/events.h"
#include <algorithm>
#include <cmath>
using namespace VSTGUI;
namespace JerzyAudio {
namespace {
const CColor bg(24,28,33),panel(34,40,47),line(63,72,81),muted(157,171,184),ink(227,234,238),amber(245,181,85),green(109,210,163);
void box(CDrawContext*c,CRect r,CColor fill,CColor edge){c->setFillColor(fill);c->setFrameColor(edge);c->setLineWidth(1);c->drawRect(r,kDrawFilledAndStroked);}
void text(CDrawContext*c,const std::string&s,CRect r,double size,CColor color,CHoriTxtAlign align=kCenterText){c->setFont(kNormalFont,size);c->setFontColor(color);c->drawString(s.c_str(),r,align);}
}
VectorControl::VectorControl(const CRect&r,IControlListener*l,int tag,Kind k,std::string s,int count)
:CControl(r,l,tag),kind(k),design(r),label(std::move(s)),steps(count){setMouseEnabled(k!=Meter);setMin(0);setMax(1);setWheelInc(.01f);setWantsFocus(k!=Meter && k!=Grip);}
void VectorControl::setText(std::string s){if(display!=s){display=std::move(s);invalid();}}
void VectorControl::draw(CDrawContext*c){
 const auto r=getViewSize();const double w=r.getWidth(),h=r.getHeight();
 CDrawContext::Transform tr(*c,CGraphicsTransform().scale(w/100.,h/100.).translate(r.left,r.top));
 c->setDrawMode(kAntiAliasing|kNonIntegralMode);
 auto caption=[&](const std::string&s,CRect area,double font,CColor color,CHoriTxtAlign align=kCenterText){
  CDrawContext::Transform unscale(*c,CGraphicsTransform().scale(100./w,100./h));
  text(c,s,{area.left*w/100.,area.top*h/100.,area.right*w/100.,area.bottom*h/100.},font*std::min(w/design.getWidth(),h/design.getHeight()),color,align);
 };
 if(kind==Knob){
  caption(label,{0,0,100,16},11,muted);
  const double cx=50,cy=48,rad=25;
  c->setFillColor(CColor(19,23,28));c->setFrameColor(line);c->setLineWidth(1.5);c->drawEllipse({cx-rad,cy-rad,cx+rad,cy+rad},kDrawFilledAndStroked);
  for(int i=0;i<=24;++i){const double a=(135+270*i/24.)*Constants::pi/180.;c->setFrameColor(i<=int(getValue()*24)?amber:line);c->setLineWidth(1.5);c->drawLine({cx+29*std::cos(a),cy+29*std::sin(a)},{cx+32*std::cos(a),cy+32*std::sin(a)});}
  const double a=(135+270*getValue())*Constants::pi/180.;c->setFrameColor(ink);c->setLineWidth(2.5);c->drawLine({cx+7*std::cos(a),cy+7*std::sin(a)},{cx+21*std::cos(a),cy+21*std::sin(a)});
  caption(display,{0,80,100,99},12,ink);
 }else if(kind==Meter){
  caption(label,{0,0,48,36},10,muted,kLeftText);caption(display,{48,0,100,36},10,ink,kRightText);
  box(c,{0,49,100,85},bg,line);
  for(int i=0;i<30;++i){c->setFillColor(i<getValue()*30?(i>26?CColor(230,101,90):i>22?amber:green):CColor(48,57,64));c->drawRect({2+i*3.2,54,4+i*3.2,80},kDrawFilled);}
 }else if(kind==Grip){c->setFrameColor(muted);c->setLineWidth(5);for(int i=0;i<3;++i)c->drawLine({35.+i*20,95},{95,35.+i*20});}
 else {
  const bool lit=kind==Choice && steps==1 && getValue()>.5f;
  box(c,{1,1,99,99},lit?CColor(99,61,30):CColor(29,34,40),lit?amber:line);
  caption(kind==Action?label:label+"  "+display,{4,4,96,96},10,lit?amber:ink);
 }
 setDirty(false);
}
CMouseEventResult VectorControl::onMouseDown(CPoint&p,const CButtonState&b){
 if(!b.isLeftButton()||kind==Meter)return kMouseEventNotHandled;
 if(kind==Action){setValue(1);valueChanged();return kMouseDownEventHandledButDontNeedMovedOrUpEvents;}
 anchor=p;startValue=getValue();
 if(kind==Grip)return kMouseEventHandled;
 beginEdit();
 if(kind==Choice){const int v=int(std::lround(getValue()*steps));setValue(float((v+1)%(steps+1))/steps);valueChanged();invalid();endEdit();return kMouseDownEventHandledButDontNeedMovedOrUpEvents;}
 if(b.isDoubleClick() || (b.getModifierState()&kControl)){setValue(getDefaultValue());valueChanged();invalid();endEdit();return kMouseDownEventHandledButDontNeedMovedOrUpEvents;}
 return kMouseEventHandled;
}
CMouseEventResult VectorControl::onMouseMoved(CPoint&p,const CButtonState&b){
 if(!b.isLeftButton())return kMouseEventNotHandled;
 if(kind==Grip){anchor=p;valueChanged();return kMouseEventHandled;}
 if(kind!=Knob || !isEditing())return kMouseEventNotHandled;
 const double fine=(b.getModifierState()&kShift)?.1:1.;
 setValue(float(std::clamp(startValue+(anchor.y-p.y)*fine/std::max(90.,getHeight()),0.,1.)));valueChanged();invalid();return kMouseEventHandled;
}
CMouseEventResult VectorControl::onMouseUp(CPoint&,const CButtonState&){if(isEditing())endEdit();return kMouseEventHandled;}
CMouseEventResult VectorControl::onMouseCancel(){if(isEditing())endEdit();return kMouseEventHandled;}
void VectorControl::onMouseWheelEvent(MouseWheelEvent&e){if(kind!=Knob && kind!=Choice)return;beginEdit();setValue(getValue()+float(e.deltaY*(steps?1./steps:(e.modifiers.has(ModifierKey::Shift)?.001:.01))));valueChanged();invalid();endEdit();e.consumed=true;}
VectorPanel::VectorPanel(const CRect&r):CView(r){setMouseEnabled(false);}
void VectorPanel::draw(CDrawContext*c){const auto r=getViewSize();CDrawContext::Transform tr(*c,CGraphicsTransform().scale(r.getWidth()/880.,r.getHeight()/560.));c->setDrawMode(kAntiAliasing|kNonIntegralMode);box(c,{0,0,880,560},bg,line);
 text(c,"JERZY TAPE DRIVE",{20,14,340,39},22,ink,kLeftText);text(c,"VECTOR 1.1  /  WORN TAPE · OPTICAL COLOUR",{20,42,450,58},10,muted,kLeftText);
 struct Section{double x,y,w,h;const char* title;};
 for(auto s:{Section{16,78,272,206,"OPTICAL INPUT"},Section{304,78,272,206,"TAPE & PREAMP"},Section{592,78,272,206,"OUTPUT"},Section{16,300,414,174,"TAPE TRANSPORT & AGE"},Section{446,300,418,174,"OUTPUT FILTERS"}}){box(c,{s.x,s.y,s.x+s.w,s.y+s.h},panel,line);text(c,s.title,{s.x+12,s.y+9,s.x+s.w-12,s.y+30},11,amber,kLeftText);}
 text(c,"Drag knob · Shift: fine · Double-click: reset",{28,454,422,469},9,muted,kLeftText);setDirty(false);}
}
