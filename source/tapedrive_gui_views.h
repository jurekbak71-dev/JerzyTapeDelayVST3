#pragma once
#include "vstgui/lib/controls/cknob.h"

namespace JerzyAudio {
// Retaining the class name keeps existing UI descriptions loadable.
class ChickenKnob : public VSTGUI::CKnob {
public:
    ChickenKnob(const VSTGUI::CRect&,VSTGUI::IControlListener*,int32_t);
    void draw(VSTGUI::CDrawContext*) override;
};
class AnalogMeter : public VSTGUI::CKnob {
public:
    AnalogMeter(const VSTGUI::CRect&,VSTGUI::IControlListener*,int32_t);
    void draw(VSTGUI::CDrawContext*) override;
};
class ToggleSwitch : public VSTGUI::CKnob {
public:
    ToggleSwitch(const VSTGUI::CRect&,VSTGUI::IControlListener*,int32_t);
    void draw(VSTGUI::CDrawContext*) override;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&,const VSTGUI::CButtonState&) override;
};
class ThreeWaySwitch : public ToggleSwitch {
public:
    ThreeWaySwitch(const VSTGUI::CRect&,VSTGUI::IControlListener*,int32_t);
    void draw(VSTGUI::CDrawContext*) override;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&,const VSTGUI::CButtonState&) override;
};
class BypassButton : public ToggleSwitch {
public:
    using ToggleSwitch::ToggleSwitch;
    void draw(VSTGUI::CDrawContext*) override;
};
class HardwarePanel : public VSTGUI::CKnob {
public:
    HardwarePanel(const VSTGUI::CRect&,VSTGUI::IControlListener*,int32_t);
    void draw(VSTGUI::CDrawContext*) override;
};
void registerTapeDriveViews();
}
