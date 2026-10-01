#include "tapedrive_editor.h"
#include "vstgui/lib/controls/ccontrol.h"

namespace JerzyAudio {

enum : int32_t {
    kZoom75Tag  = 9001,
    kZoom100Tag = 9002,
    kZoom125Tag = 9003,
    kZoom150Tag = 9004
};

void TapeDriveEditor::applyZoom(double factor)
{
    if (factor == getZoomFactor()) return;
    setZoomFactor(factor);
    const double absScale = getAbsScaleFactor();
    requestResize({1200.0 * absScale, 672.0 * absScale});
}

void TapeDriveEditor::valueChanged(VSTGUI::CControl* control)
{
    if (control) {
        switch (control->getTag()) {
            case kZoom75Tag:  applyZoom(0.75); return;
            case kZoom100Tag: applyZoom(1.00); return;
            case kZoom125Tag: applyZoom(1.25); return;
            case kZoom150Tag: applyZoom(1.50); return;
            default: break;
        }
    }
    VSTGUI::VST3Editor::valueChanged(control);
}

}
