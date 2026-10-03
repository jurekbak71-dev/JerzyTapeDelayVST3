#pragma once
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/controls/icontrollistener.h"
#include <string>
namespace JerzyAudio {
class VectorControl final : public VSTGUI::CControl {
public:
 enum Kind { Knob, Choice, Action, Meter, Grip };
 VectorControl(const VSTGUI::CRect&,VSTGUI::IControlListener*,int,Kind,std::string,int steps=0);
 VSTGUI::CBaseObject* newCopy() const override {return new VectorControl(*this);}
 void draw(VSTGUI::CDrawContext*) override;
 VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&,const VSTGUI::CButtonState&) override;
 VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint&,const VSTGUI::CButtonState&) override;
 VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint&,const VSTGUI::CButtonState&) override;
 VSTGUI::CMouseEventResult onMouseCancel() override;
 void onMouseWheelEvent(VSTGUI::MouseWheelEvent&) override;
 void setText(std::string);
 Kind kind; VSTGUI::CRect design; std::string label,display; int steps;
 VSTGUI::CPoint anchor; float startValue=0;
};
class VectorPanel final : public VSTGUI::CView {
public:
 explicit VectorPanel(const VSTGUI::CRect&);
 void draw(VSTGUI::CDrawContext*) override;
};
}
