#pragma once
#include "vstgui/lib/controls/cknob.h"
#include "vstgui/lib/controls/ccontrol.h"

namespace JerzyAudio {

class ChickenKnob : public VSTGUI::CKnob {
public:
    ChickenKnob(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag);
    void draw(VSTGUI::CDrawContext* context) override;
};

class AnalogMeter : public VSTGUI::CControl {
public:
    AnalogMeter(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag);
    void draw(VSTGUI::CDrawContext* context) override;
};

void registerTapeDriveViews();

}
