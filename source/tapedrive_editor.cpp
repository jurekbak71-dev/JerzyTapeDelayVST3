#include "tapedrive_editor.h"
#include "tapedrive_editor_geometry.h"
#include "vstgui/lib/cframe.h"
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
    factor=std::clamp(factor,TapeDriveEditorGeometry::minZoom,TapeDriveEditorGeometry::maxZoom);
    if (factor == getZoomFactor()) return;
    const double previous=getZoomFactor();
    // CFrame::setZoom requests the host resize itself. A second request could
    // leave the editor's scale and the accepted host window size out of sync.
    setZoomFactor(factor);
    if(getFrame() && std::abs(getFrame()->getZoom()-getAbsScaleFactor())>1e-6)
        setZoomFactor(previous); // Keep the old scale if the host rejected resizing.

}

Steinberg::tresult PLUGIN_API TapeDriveEditor::checkSizeConstraint(Steinberg::ViewRect* rect)
{
    if(!rect)return Steinberg::kInvalidArgument;
    const double dpi=getContentScaleFactor();
    const double zoom=TapeDriveEditorGeometry::zoomForWidth(rect->getWidth(),dpi);
    rect->right=rect->left+TapeDriveEditorGeometry::pixelWidth(zoom,dpi);
    rect->bottom=rect->top+TapeDriveEditorGeometry::pixelHeight(zoom,dpi);
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API TapeDriveEditor::onSize(Steinberg::ViewRect* rect)
{
    if(!rect)return Steinberg::kInvalidArgument;
    if(hostResizing)return VSTGUI::VST3Editor::onSize(rect);
    auto constrained=*rect;
    checkSizeConstraint(&constrained);
    if(std::abs(constrained.getWidth()-rect->getWidth())>1 ||
       std::abs(constrained.getHeight()-rect->getHeight())>1)
        return Steinberg::kResultFalse;
    // Scale the content first, with resize callbacks suppressed while the host
    // is already resizing. Controls retain their coordinates and hit areas.
    hostResizing=true;
    setZoomFactor(TapeDriveEditorGeometry::zoomForWidth(rect->getWidth(),getContentScaleFactor()));
    const auto result=VSTGUI::VST3Editor::onSize(rect);
    hostResizing=false;
    return result;
}

bool TapeDriveEditor::beforeSizeChange(const VSTGUI::CRect& newSize,const VSTGUI::CRect& oldSize)
{
    return hostResizing || VSTGUI::VST3Editor::beforeSizeChange(newSize,oldSize);
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
