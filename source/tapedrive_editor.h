#pragma once
#include "vstgui/plugin-bindings/vst3editor.h"

namespace JerzyAudio {

class TapeDriveEditor : public VSTGUI::VST3Editor {
public:
    using VSTGUI::VST3Editor::VST3Editor;
    void valueChanged(VSTGUI::CControl* control) override;

private:
    void applyZoom(double factor);
};

}
