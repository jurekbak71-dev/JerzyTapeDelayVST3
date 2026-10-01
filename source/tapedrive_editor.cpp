#include "tapedrive_editor.h"
#include "tapedrive_editor_geometry.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/cgraphicstransform.h"
#include "vstgui/lib/controls/ccontrol.h"

namespace JerzyAudio {

bool PLUGIN_API TapeDriveEditor::open(void* parent,const VSTGUI::PlatformType& type)
{
    if(!VSTGUI::VST3Editor::open(parent,type))return false;
    // VST3Editor recreates the frame transform while loading its template.
    // Reconcile it with the actual host rectangle after the template exists.
    fitHostSize(getRect());
    return true;
}

void TapeDriveEditor::fitHostSize(const Steinberg::ViewRect& size)
{
    auto* frame=getFrame();
    if(!frame || size.getWidth()<=0 || size.getHeight()<=0)return;
    const bool previous=applyingSize;
    applyingSize=true;
    frame->setAutosizingEnabled(false);
    // The template and its mouse areas stay in fixed design coordinates.
    // The same transform scales the background, drawing and hit testing.
    frame->forEachChild([](const auto& view){
        if(auto* root=view->asViewContainer()){
            root->setAutosizingEnabled(false);
            const VSTGUI::CRect design(0,0,TapeDriveEditorGeometry::width,TapeDriveEditorGeometry::height);
            root->setViewSize(design);
            root->setMouseableArea(design);
        }
    });
    frame->setSize(size.getWidth(),size.getHeight());
    frame->setTransform(VSTGUI::CGraphicsTransform().scale(
        size.getWidth()/TapeDriveEditorGeometry::width,
        size.getHeight()/TapeDriveEditorGeometry::height));
    frame->setAutosizingEnabled(true);
    frame->invalid();
    applyingSize=previous;
}

void TapeDriveEditor::applyZoom(double factor)
{
    factor=std::clamp(factor,TapeDriveEditorGeometry::minZoom,TapeDriveEditorGeometry::maxZoom);
    const double dpi=getContentScaleFactor();
    const VSTGUI::CPoint target(TapeDriveEditorGeometry::pixelWidth(factor,dpi),
                                TapeDriveEditorGeometry::pixelHeight(factor,dpi));
    // Ask the host first. Do not change CFrame's cached zoom before its resize
    // callback; hosts may re-enter onSize or reject this request.
    requestResize(target);
    fitHostSize(getRect());
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
    if(!rect || rect->getWidth()<=0 || rect->getHeight()<=0)return Steinberg::kInvalidArgument;
    // Some hosts resize without checkSizeConstraint. Always fit their accepted
    // client rectangle instead of rejecting it and leaving an unscaled panel.
    userZoom=rect->getWidth()/(TapeDriveEditorGeometry::width*TapeDriveEditorGeometry::dpi(getContentScaleFactor()));
    setRect(*rect);
    fitHostSize(*rect);
    return Steinberg::kResultTrue;
}

#ifdef VST3_CONTENT_SCALE_SUPPORT
Steinberg::tresult PLUGIN_API TapeDriveEditor::setContentScaleFactor(ScaleFactor factor)
{
    if(!std::isfinite(factor) || factor<=0)return Steinberg::kInvalidArgument;
    const auto previousRect=getRect();
    applyingSize=true;
    const auto result=VSTGUI::VST3Editor::setContentScaleFactor(factor);
    applyingSize=false;
    if(result!=Steinberg::kResultOk)return result;
    const Steinberg::ViewRect target(0,0,
        TapeDriveEditorGeometry::pixelWidth(userZoom,factor),
        TapeDriveEditorGeometry::pixelHeight(userZoom,factor));
    if(!getFrame()){
        setRect(target); // Required when DPI is supplied before attached/setFrame.
    }else{
        // Undo the base editor's eager frame resize, then use the host callback.
        fitHostSize(previousRect);
        requestResize({static_cast<double>(target.getWidth()),static_cast<double>(target.getHeight())});
        fitHostSize(getRect());
    }
    return result;
}
#endif

bool TapeDriveEditor::beforeSizeChange(const VSTGUI::CRect& newSize,const VSTGUI::CRect& oldSize)
{
    return applyingSize || VSTGUI::VST3Editor::beforeSizeChange(newSize,oldSize);
}

void TapeDriveEditor::valueChanged(VSTGUI::CControl* control)
{
    if(control && control->getTag()>=9001 && control->getTag()<=9004){
        if(control->getValueNormalized()>0.5f){
            const double factors[]={0.75,1.0,1.25,1.5};
            applyZoom(factors[control->getTag()-9001]);
        }
        return;
    }
    VSTGUI::VST3Editor::valueChanged(control);
}
}
