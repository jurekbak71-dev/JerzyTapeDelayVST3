#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/ivstplugview.h"
#include "../source/mxdelay_params.h"
#include <windows.h>
#include <objbase.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace Steinberg;using namespace Steinberg::Vst;using namespace JerzyAudio;
using RenderProbe=int(__cdecl*)(IPlugView*,const char*);
RenderProbe renderProbe=nullptr;
void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
class HostFrame final:public U::Implements<U::Directly<IPlugFrame>>{
public:HWND window=nullptr;tresult PLUGIN_API resizeView(IPlugView* view,ViewRect* size) override{MoveWindow(window,0,0,size->getWidth(),size->getHeight(),FALSE);return view->onSize(size);}};
void pump(){MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
HWND editorWindow(HWND parent){HWND largest=nullptr;long area=0;for(HWND child=GetWindow(parent,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){RECT r{};GetClientRect(child,&r);long a=r.right*r.bottom;if(a>area){area=a;largest=child;}}require(largest,"Native editor child missing");return largest;}
ViewRect sizeOf(IPlugView* view){ViewRect r;require(view->getSize(&r)==kResultTrue,"getSize failed");return r;}
void mouse(IPlugView* view,HWND parent,double x,double y,UINT down,UINT up,WPARAM state){
 auto r=sizeOf(view);int px=(int)std::lround(x*r.getWidth()/1280.0),py=(int)std::lround(y*r.getHeight()/720.0);HWND child=editorWindow(parent);
 SendMessageW(child,WM_MOUSEMOVE,0,MAKELPARAM(px,py));SendMessageW(child,down,state,MAKELPARAM(px,py));SendMessageW(child,up,0,MAKELPARAM(px,py));pump();
}
void left(IPlugView*v,HWND p,double x,double y){mouse(v,p,x,y,WM_LBUTTONDOWN,WM_LBUTTONUP,MK_LBUTTON);}
void right(IPlugView*v,HWND p,double x,double y){mouse(v,p,x,y,WM_RBUTTONDOWN,WM_RBUTTONUP,MK_RBUTTON);}
void setParam(IEditController* controller,ParamID id,double value){
 require(controller->setParamNormalized(id,value)==kResultTrue,"setParamNormalized failed");
}
void checkFinder(IPlugView*view){
 FUnknownPtr<IParameterFinder> finder(view);require(finder!=nullptr,"No IParameterFinder");auto r=sizeOf(view);ParamID id=0;
 auto find=[&](double x,double y,ParamID expected){require(finder->findParameter((int32)std::lround(x*r.getWidth()/1280.0),(int32)std::lround(y*r.getHeight()/720.0),id)==kResultTrue&&id==expected,"Scaled hit area mismatch");};
 find(55,91,kMXMixId);find(93,553,slotParam(0,kSlotHeadPan1));find(418,230,slotParam(0,kSlotTime));find(500,230,slotParam(0,kSlotFeedback));
}
int main(int argc,char**argv){try{
 require(argc==2,"Pass JerzyMXAnalogDelay.vst3 path");std::cerr<<"mxgui: start\n";SetProcessDPIAware();CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
 std::cerr<<"mxgui: COM ready\n";std::string error;auto module=VST3::Hosting::Module::create(argv[1],error);if(!module)throw std::runtime_error(error);
 std::cerr<<"mxgui: module loaded\n";auto native=GetModuleHandleA(argv[1]);renderProbe=reinterpret_cast<RenderProbe>(GetProcAddress(native,"JerzyRenderMXEditorForTest"));require(renderProbe,"MX render probe missing");
 std::cerr<<"mxgui: probe ready\n";HostApplication host;module->getFactory().setHostContext(&host);IPtr<IEditController> controller;
 for(const auto& info:module->getFactory().classInfos())if(info.category()==kVstComponentControllerClass)controller=module->getFactory().createInstance<IEditController>(info.ID());
 require(controller&&controller->initialize(&host)==kResultOk,"MX controller initialization failed");std::cerr<<"mxgui: controller initialized\n";
 int automatable=0;for(int32 i=0;i<controller->getParameterCount();++i){ParameterInfo info{};require(controller->getParameterInfo(i,info)==kResultTrue,"ParameterInfo failed");if(info.flags&ParameterInfo::kCanAutomate)++automatable;}require(automatable>80,"Too few FL-automatable parameters");
 std::cerr<<"mxgui: params checked\n";FUnknownPtr<IMidiMapping> midi(controller);require(midi!=nullptr,"IMidiMapping missing");ParamID mapped=0;require(midi->getMidiControllerAssignment(0,0,20,mapped)==kResultTrue&&mapped==slotParam(0,kSlotHeadPan1),"CC20 head pan mapping missing");require(midi->getMidiControllerAssignment(0,0,43,mapped)==kResultTrue&&mapped==algoParam(1,(int)DelayAlgorithm::Volante,13),"CC43 feedback-head mapping missing");
 std::cerr<<"mxgui: midi checked\n";auto view=owned(controller->createView(ViewType::kEditor));require(view,"createView failed");HostFrame frame;frame.window=CreateWindowExW(0,L"STATIC",L"MX GUI test",WS_POPUP,0,0,1280,720,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);require(frame.window,"Host window failed");
 std::cerr<<"mxgui: host hwnd ready\n";FUnknownPtr<IPlugViewContentScaleSupport> scale(view);require(scale!=nullptr,"No DPI support");std::cerr<<"mxgui: setting preattach dpi\n";require(scale->setContentScaleFactor(1.25f)==kResultTrue,"Pre-attach DPI failed");std::cerr<<"mxgui: preattach dpi ok\n";auto initial=sizeOf(view);require(initial.getWidth()==1600&&initial.getHeight()==900,"Wrong pre-attach scale");
 MoveWindow(frame.window,0,0,initial.getWidth(),initial.getHeight(),FALSE);view->setFrame(&frame);std::cerr<<"mxgui: attaching\n";require(view->attached(frame.window,kPlatformTypeHWND)==kResultTrue,"Attach failed");std::cerr<<"mxgui: attached\n";pump();checkFinder(view);std::cerr<<"mxgui: finder ok\n";
 std::cerr<<"mxgui: interaction start\n";setParam(controller,kMXMixId,0.82);pump();require(std::abs(controller->getParamNormalized(kMXMixId)-0.82)<.002,"VST3 parameter change did not reach controller");right(view,frame.window,55,91);require(std::abs(controller->getParamNormalized(kMXMixId)-0.35)<.002,"Right-click did not reset knob to default");
 std::cerr<<"mxgui: right reset ok\n";double en=controller->getParamNormalized(slotParam(0,kSlotEnable));left(view,frame.window,201,225);require(controller->getParamNormalized(slotParam(0,kSlotEnable))!=en,"LED switch click failed");left(view,frame.window,201,225);
 std::cerr<<"mxgui: led ok\n";for(const auto& z: {std::pair<double,double>{690,79}, {745,79}, {800,79}, {855,79}}){left(view,frame.window,z.first,z.second);checkFinder(view);}
 std::cerr<<"mxgui: zooms ok\n";ViewRect restored(0,0,1024,576);require(frame.resizeView(view,&restored)==kResultTrue,"Host resize rejected");checkFinder(view);require(renderProbe(view,"mxdelay-render-1024x576.png")==1,"Rendered MX GUI has blank/unpainted edges");
 std::cerr<<"mxgui: render small ok\n";restored=ViewRect(0,0,1920,1080);require(frame.resizeView(view,&restored)==kResultTrue,"Large host resize rejected");checkFinder(view);require(renderProbe(view,"mxdelay-render-1920x1080.png")==1,"Large MX render invalid");
 std::cerr<<"mxgui: render large ok\n";view->removed();view->setFrame(nullptr);scale=nullptr;view=nullptr;DestroyWindow(frame.window);controller->terminate();controller=nullptr;module.reset();CoUninitialize();
 std::cout<<"MX GUI scaling, automation, MIDI, hit areas, LED switch and right-click reset OK\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
