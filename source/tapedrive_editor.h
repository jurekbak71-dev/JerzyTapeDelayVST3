#pragma once
#include "vstgui/plugin-bindings/vst3editor.h"

namespace JerzyAudio {

class TapeDriveEditor : public VSTGUI::AspectRatioVST3Editor {
public:
    using VSTGUI::AspectRatioVST3Editor::AspectRatioVST3Editor;
    void valueChanged(VSTGUI::CControl* control) override;

private:
    void applyZoom(double factor);
};

}
