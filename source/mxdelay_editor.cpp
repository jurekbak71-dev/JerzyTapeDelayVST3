#include "mxdelay_editor.h"
#include "mxdelay_editor_geometry.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/cgraphicstransform.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/platform/iplatformframe.h"
#include <algorithm>
#include <cmath>
#if defined(_WIN32)
#include <windows.h>
#include <commctrl.h>
#endif

namespace JerzyAudio {
struct MXDelayEditor::NativeResizeWatcher {
#if defined(_WIN32)
    MXDelayEditor& editor;
    HWND parent=nullptr,child=nullptr;
    UINT_PTR timer=0;
    RECT lastParent{},lastChild{};
    NativeResizeWatcher(MXDelayEditor& e,void* p,void* c):editor(e),parent(static_cast<HWND>(p)),child(static_cast<HWND>(c)){
        SetWindowSubclass(parent,callback,reinterpret_cast<UINT_PTR>(this),reinterpret_cast<DWORD_PTR>(this));
        SetWindowSubclass(child,callback,reinterpret_cast<UINT_PTR>(this),reinterpret_cast<DWORD_PTR>(this));
        GetClientRect(parent,&lastParent);GetClientRect(child,&lastChild);
        timer=SetTimer(child,reinterpret_cast<UINT_PTR>(this),100,nullptr);
    }
    ~NativeResizeWatcher(){
        if(child&&timer)KillTimer(child,timer);
        if(parent)RemoveWindowSubclass(parent,callback,reinterpret_cast<UINT_PTR>(this));
        if(child)RemoveWindowSubclass(child,callback,reinterpret_cast<UINT_PTR>(this));
    }
    void fit(HWND window){
        if(editor.applyingSize||!editor.getFrame())return;
        RECT r{};if(!GetClientRect(window,&r)||r.right<=0||r.bottom<=0)return;
        const auto current=editor.getRect();
        const auto transform=editor.getFrame()->getTransform();
        const auto bounds=editor.getFrame()->getViewSize();
        const bool sized=current.getWidth()==r.right&&current.getHeight()==r.bottom&&bounds.getWidth()==r.right&&bounds.getHeight()==r.bottom;
        const bool scaled=std::abs(transform.m11-r.right/MXDelayEditorGeometry::width)<1e-8&&std::abs(transform.m22-r.bottom/MXDelayEditorGeometry::height)<1e-8;
        if(sized&&scaled)return;
        Steinberg::ViewRect rect(0,0,r.right,r.bottom);editor.onSize(&rect);
    }
    void reconcile(){
        if(editor.applyingSize||!parent||!child)return;
        RECT p{},c{};if(!GetClientRect(parent,&p)||!GetClientRect(child,&c))return;
        const bool parentChanged=p.right!=lastParent.right||p.bottom!=lastParent.bottom;
        const bool childChanged=c.right!=lastChild.right||c.bottom!=lastChild.bottom;
        if(parentChanged)fit(parent);else if(childChanged)fit(child);else{
            const auto size=editor.getRect();const auto transform=editor.getFrame()->getTransform();
            if(std::abs(transform.m11-size.getWidth()/MXDelayEditorGeometry::width)>1e-8||
               std::abs(transform.m22-size.getHeight()/MXDelayEditorGeometry::height)>1e-8)editor.fitHostSize(size);
        }
        GetClientRect(parent,&lastParent);GetClientRect(child,&lastChild);
    }
    static LRESULT CALLBACK callback(HWND window,UINT message,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR data){
        auto*self=reinterpret_cast<NativeResizeWatcher*>(data);
        if(message==WM_NCDESTROY){
            RemoveWindowSubclass(window,callback,id);
            if(window==self->parent)self->parent=nullptr;if(window==self->child)self->child=nullptr;
            return DefSubclassProc(window,message,w,l);
        }
        const auto result=DefSubclassProc(window,message,w,l);
        if(message==WM_SIZE&&w!=SIZE_MINIMIZED)self->fit(window);
        if(message==WM_WINDOWPOSCHANGED||message==WM_DPICHANGED_AFTERPARENT||(message==WM_TIMER&&w==self->timer))self->reconcile();
        return result;
    }
#else
    NativeResizeWatcher(MXDelayEditor&,void*,void*){}
#endif
};
MXDelayEditor::~MXDelayEditor(){delete nativeWatcher;}
void PLUGIN_API MXDelayEditor::close(){delete nativeWatcher;nativeWatcher=nullptr;VSTGUI::VST3Editor::close();}
bool PLUGIN_API MXDelayEditor::open(void* parent,const VSTGUI::PlatformType& type){
    if(!VSTGUI::VST3Editor::open(parent,type))return false;
    fitHostSize(getRect());
#if defined(_WIN32)
    delete nativeWatcher;
    nativeWatcher=new NativeResizeWatcher(*this,parent,getFrame()->getPlatformFrame()->getPlatformRepresentation());
    nativeWatcher->fit(static_cast<HWND>(parent));
#endif
    return true;
}
void MXDelayEditor::fitHostSize(const Steinberg::ViewRect& size){
    auto*frame=getFrame();if(!frame||size.getWidth()<=0||size.getHeight()<=0)return;
    const bool previous=applyingSize;applyingSize=true;frame->setAutosizingEnabled(false);
    frame->forEachChild([](const auto& view){
        if(auto*root=view->asViewContainer()){
            root->setAutosizingEnabled(false);
            const VSTGUI::CRect design(0,0,MXDelayEditorGeometry::width,MXDelayEditorGeometry::height);
            root->setViewSize(design);root->setMouseableArea(design);
        }
    });
    frame->setTransform(VSTGUI::CGraphicsTransform().scale(size.getWidth()/MXDelayEditorGeometry::width,size.getHeight()/MXDelayEditorGeometry::height));
    frame->setSize(size.getWidth(),size.getHeight());frame->setAutosizingEnabled(true);frame->invalid();applyingSize=previous;
}
void MXDelayEditor::applyZoom(double factor){
    factor=std::clamp(factor,MXDelayEditorGeometry::minZoom,MXDelayEditorGeometry::maxZoom);
    const double dpi=getContentScaleFactor();
    const VSTGUI::CPoint target(MXDelayEditorGeometry::pixelWidth(factor,dpi),MXDelayEditorGeometry::pixelHeight(factor,dpi));
    requestResize(target);fitHostSize(getRect());
}
Steinberg::tresult PLUGIN_API MXDelayEditor::checkSizeConstraint(Steinberg::ViewRect* rect){
    if(!rect)return Steinberg::kInvalidArgument;const double dpi=getContentScaleFactor();
    const double zoom=MXDelayEditorGeometry::zoomForWidth(rect->getWidth(),dpi);
    rect->right=rect->left+MXDelayEditorGeometry::pixelWidth(zoom,dpi);
    rect->bottom=rect->top+MXDelayEditorGeometry::pixelHeight(zoom,dpi);
    return Steinberg::kResultTrue;
}
Steinberg::tresult PLUGIN_API MXDelayEditor::onSize(Steinberg::ViewRect* rect){
    if(!rect||rect->getWidth()<=0||rect->getHeight()<=0)return Steinberg::kInvalidArgument;
    userZoom=rect->getWidth()/(MXDelayEditorGeometry::width*MXDelayEditorGeometry::dpi(getContentScaleFactor()));
    setRect(*rect);fitHostSize(*rect);return Steinberg::kResultTrue;
}
#ifdef VST3_CONTENT_SCALE_SUPPORT
Steinberg::tresult PLUGIN_API MXDelayEditor::setContentScaleFactor(ScaleFactor factor){
    if(!std::isfinite(factor)||factor<=0)return Steinberg::kInvalidArgument;
    const auto previousRect=getRect();applyingSize=true;const auto result=VSTGUI::VST3Editor::setContentScaleFactor(factor);applyingSize=false;
    if(result!=Steinberg::kResultOk)return result;
    const Steinberg::ViewRect target(0,0,MXDelayEditorGeometry::pixelWidth(userZoom,factor),MXDelayEditorGeometry::pixelHeight(userZoom,factor));
    if(!getFrame())setRect(target);else{fitHostSize(previousRect);requestResize({static_cast<double>(target.getWidth()),static_cast<double>(target.getHeight())});fitHostSize(getRect());}
    return result;
}
#endif
bool MXDelayEditor::beforeSizeChange(const VSTGUI::CRect& newSize,const VSTGUI::CRect& oldSize){return applyingSize||VSTGUI::VST3Editor::beforeSizeChange(newSize,oldSize);}
void MXDelayEditor::valueChanged(VSTGUI::CControl* control){
    if(control&&control->getTag()>=9101&&control->getTag()<=9104){
        if(control->getValueNormalized()>0.5f){const double factors[]={0.75,1.0,1.25,1.5};applyZoom(factors[control->getTag()-9101]);}
        return;
    }
    VSTGUI::VST3Editor::valueChanged(control);
}
}
