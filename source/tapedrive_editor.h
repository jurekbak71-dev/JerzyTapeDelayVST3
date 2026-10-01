#pragma once
#include "vstgui/plugin-bindings/vst3editor.h"

namespace JerzyAudio {

class TapeDriveEditor : public VSTGUI::VST3Editor {
public:
    using VSTGUI::VST3Editor::VST3Editor;
    void valueChanged(VSTGUI::CControl* control) override;

protected:
    Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect* rect) override;
    Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* rect) override;
    bool beforeSizeChange(const VSTGUI::CRect& newSize,const VSTGUI::CRect& oldSize) override;

private:
    bool hostResizing=false;
    void applyZoom(double factor);
};

}
