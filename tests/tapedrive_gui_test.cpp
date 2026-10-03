// Real VST3 DLL, visible native Windows editor, controller edits and desktop pixels.
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstplugview.h"
#include "../source/tapedrive_params.h"
#include <windows.h>
#include <objbase.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace Steinberg;using namespace Steinberg::Vst;using namespace JerzyAudio;
using Probe=int(__cdecl*)(IPlugView*,const char*);Probe visibleProbe,renderProbe;
void require(bool b,const char*s){if(!b)throw std::runtime_error(s);}
class HostFrame final:public U::Implements<U::Directly<IPlugFrame>>{public:HWND window=nullptr;bool reject=false;tresult PLUGIN_API resizeView(IPlugView*v,ViewRect*r)override{if(reject)return kResultFalse;MoveWindow(window,0,0,r->getWidth(),r->getHeight(),FALSE);return v->onSize(r);}};
class Edits final:public U::Implements<U::Directly<IComponentHandler>>{public:int begins=0,ends=0,changes=0;tresult PLUGIN_API beginEdit(ParamID)override{++begins;return kResultOk;}tresult PLUGIN_API endEdit(ParamID)override{++ends;return kResultOk;}tresult PLUGIN_API performEdit(ParamID,ParamValue)override{++changes;return kResultOk;}tresult PLUGIN_API restartComponent(int32)override{return kResultOk;}};
void pump(){MSG m;while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}}
void settle(){auto end=GetTickCount64()+180;do{pump();MsgWaitForMultipleObjects(0,nullptr,FALSE,10,QS_ALLINPUT);}while(GetTickCount64()<end);}
ViewRect sizeOf(IPlugView*v){ViewRect r;require(v->getSize(&r)==kResultTrue,"getSize failed");return r;}
HWND child(HWND p){HWND c=GetWindow(p,GW_CHILD);require(c!=nullptr,"Missing native editor");return c;}
POINT point(IPlugView*v,double x,double y){auto r=sizeOf(v);return {LONG(std::lround(x*r.getWidth()/880.)),LONG(std::lround(y*r.getHeight()/560.))};}
void click(IPlugView*v,HWND p,double x,double y){auto q=point(v,x,y);SendMessageW(child(p),WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(q.x,q.y));SendMessageW(child(p),WM_LBUTTONUP,0,MAKELPARAM(q.x,q.y));pump();}
void check(IPlugView*v,IEditController*c,HWND p){auto r=sizeOf(v);RECT native{};GetClientRect(child(p),&native);require(native.right==r.getWidth()&&native.bottom==r.getHeight(),"Child and host sizes disagree");FUnknownPtr<IParameterFinder> finder(v);require(finder!=nullptr,"Missing parameter finder");
 struct Hit{double x,y;ParamID id;};
 for(auto h:{Hit{66,155,kOptoAmountId},Hit{152,155,kOptoColorId},Hit{238,155,kOptoMakeupId},Hit{66,238,kOptoRecoveryId},Hit{152,238,kOptoMixId},Hit{374,177,kSatId},Hit{502,177,kPreampDriveId},Hit{238,222,kOptoBypassId},Hit{387,258,kPreampModeId},Hit{515,258,kGainModeId},Hit{662,177,kLevelId},Hit{790,177,kDryId},Hit{726,258,kDriveBypassId},Hit{88,390,kWowId},Hit{222,390,kFlutterId},Hit{356,390,kTapeAgeId},Hit{502,386,kHPFCutoffId},Hit{604,386,kHPFResId},Hit{706,386,kLPFCutoffId},Hit{808,386,kLPFResId},Hit{650,457,kShiftId}}){auto q=point(v,h.x,h.y);ParamID id=0;require(finder->findParameter(q.x,q.y,id)==kResultTrue&&id==h.id,"Scaled control cannot be found");}
 auto before=c->getParamNormalized(kDriveBypassId);click(v,p,726,258);require(c->getParamNormalized(kDriveBypassId)!=before,"Bypass did not change");click(v,p,726,258);require(c->getParamNormalized(kDriveBypassId)==before,"Bypass did not restore");
}
int main(int argc,char**argv){try{require(argc==2,"Pass VST3 module path");SetProcessDPIAware();CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);std::string error;auto module=VST3::Hosting::Module::create(argv[1],error);if(!module)throw std::runtime_error(error);auto native=GetModuleHandleA(argv[1]);visibleProbe=reinterpret_cast<Probe>(GetProcAddress(native,"JerzyCaptureVisibleEditorForTest"));renderProbe=reinterpret_cast<Probe>(GetProcAddress(native,"JerzyRenderEditorForTest"));require(visibleProbe&&renderProbe,"Render probes missing");HostApplication host;module->getFactory().setHostContext(&host);IPtr<IEditController> c;for(const auto&i:module->getFactory().classInfos())if(i.category()==kVstComponentControllerClass)c=module->getFactory().createInstance<IEditController>(i.ID());require(c&&c->initialize(&host)==kResultOk,"Controller init failed");Edits edits;c->setComponentHandler(&edits);auto v=owned(c->createView(ViewType::kEditor));require(v!=nullptr,"Editor creation failed");auto initial=sizeOf(v);require(initial.getWidth()==880&&initial.getHeight()==560,"Default viewport is not compact");HostFrame f;f.window=CreateWindowExW(0,L"STATIC",L"Tape Drive Vector 1.1",WS_POPUP,0,0,880,560,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);v->setFrame(&f);FUnknownPtr<IPlugViewContentScaleSupport> dpi(v);require(dpi!=nullptr,"DPI interface missing");dpi->setContentScaleFactor(2.f);initial=sizeOf(v);require(initial.getWidth()==880&&initial.getHeight()==560,"DPI multiplied default size");
 // Oversized size from an old FL session must be clamped when opening.
 ViewRect old(0,0,2500,1600);v->onSize(&old);MoveWindow(f.window,0,0,2500,1600,FALSE);require(v->attached(f.window,kPlatformTypeHWND)==kResultTrue,"Cannot open GUI");settle();initial=sizeOf(v);require(initial.getWidth()<GetSystemMetrics(SM_CXSCREEN)&&initial.getHeight()<GetSystemMetrics(SM_CYSCREEN),"Restored GUI exceeds screen");check(v,c,f.window);
 for(float scale:{1.f,1.25f,1.5f,2.f}){auto prev=sizeOf(v);dpi->setContentScaleFactor(scale);auto after=sizeOf(v);require(prev.getWidth()==after.getWidth()&&prev.getHeight()==after.getHeight(),"DPI changed physical viewport unexpectedly");
 for(double x:{576.,654.,732.,810.}){click(v,f.window,x,34);settle();check(v,c,f.window);auto r=sizeOf(v);require(r.getHeight()<GetSystemMetrics(SM_CYSCREEN),"Zoom exceeds screen height");}
 // Resize directly without VST3 callback; timer must relayout actual controls.
 MoveWindow(f.window,0,0,704,448,FALSE);settle();auto r=sizeOf(v);require(r.getWidth()==704&&r.getHeight()==448,"Native resize was ignored");check(v,c,f.window);
 f.reject=true;auto accepted=sizeOf(v);click(v,f.window,810,34);require(sizeOf(v).getWidth()==accepted.getWidth(),"Rejected resize altered panel");f.reject=false;
 }
 // Genuine drag sends beginEdit/performEdit/endEdit and changes the DSP parameter.
 auto q=point(v,374,177);double before=c->getParamNormalized(kSatId);SendMessageW(child(f.window),WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(q.x,q.y));SendMessageW(child(f.window),WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(q.x,q.y-25));SendMessageW(child(f.window),WM_LBUTTONUP,0,MAKELPARAM(q.x,q.y-25));require(c->getParamNormalized(kSatId)>before,"Knob drag did not edit parameter");require(edits.changes>0&&edits.begins==edits.ends,"Automation gesture imbalance");
 // Grip must request host resize, using the same path as top-row zoom buttons.
 auto a=point(v,866,546),b=point(v,810,500);int widthBefore=sizeOf(v).getWidth();SendMessageW(child(f.window),WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(a.x,a.y));SendMessageW(child(f.window),WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(b.x,b.y));SendMessageW(child(f.window),WM_LBUTTONUP,0,MAKELPARAM(b.x,b.y));require(sizeOf(v).getWidth()!=widthBefore,"Resize grip did not work");check(v,c,f.window);
 // Screenshot actual visible pixels at minimum and default sizes.
 for(auto dimensions:{std::pair<int,int>{616,392},{880,560}}){ViewRect r(0,0,dimensions.first,dimensions.second);f.resizeView(v,&r);SetWindowPos(f.window,HWND_TOPMOST,0,0,r.getWidth(),r.getHeight(),SWP_SHOWWINDOW);ShowWindow(f.window,SW_SHOW);settle();check(v,c,f.window);std::string path="tapedrive-window-"+std::to_string(r.getWidth())+".bmp";require(visibleProbe(v,path.c_str())==1,"Visible vector panel differs from expected rendering");require(renderProbe(v,"tapedrive-render-vector.png")==1,"Unpainted panel edges");}
 v->removed();v->setFrame(nullptr);dpi=nullptr;v=nullptr;DestroyWindow(f.window);c->setComponentHandler(nullptr);c->terminate();c=nullptr;module.reset();CoUninitialize();std::cout<<"Vector GUI: compact startup, restored-size clamp, all controls, drag, automation, grip, zoom, DPI, native resize and visible pixels OK\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
