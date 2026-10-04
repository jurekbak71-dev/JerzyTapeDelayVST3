#include "../source/mxdelay_editor.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/coffscreencontext.h"
#include "vstgui/lib/cbitmap.h"
#include "vstgui/lib/platform/platformfactory.h"
#include "vstgui/lib/platform/iplatformframe.h"
#include "vstgui/lib/cgraphicstransform.h"
#include <fstream>
#include <windows.h>

extern "C" __declspec(dllexport) int __cdecl JerzyRenderMXEditorForTest(Steinberg::IPlugView* view,const char* path){
    auto* editor=dynamic_cast<JerzyAudio::MXDelayEditor*>(view);
    if(!editor||!editor->getFrame())return 0;
    auto* frame=editor->getFrame();const auto rect=frame->getViewSize();
    auto context=VSTGUI::COffscreenContext::create({rect.getWidth(),rect.getHeight()});if(!context)return 0;
    context->beginDraw();context->setFillColor(VSTGUI::CColor(255,0,255));context->drawRect(rect,VSTGUI::kDrawFilled);frame->drawRect(context,rect);context->endDraw();
    auto*bitmap=context->getBitmap();const auto png=VSTGUI::getPlatformFactory().createBitmapMemoryPNGRepresentation(bitmap->getPlatformBitmap());
    std::ofstream image(path,std::ios::binary);image.write(reinterpret_cast<const char*>(png.data()),png.size());
    auto pixels=VSTGUI::owned(VSTGUI::CBitmapPixelAccess::create(bitmap));if(!pixels)return 0;
    for(double y:{.02,.50,.98})for(double x:{.02,.50,.98}){pixels->setPosition((uint32_t)(x*rect.getWidth()),(uint32_t)(y*rect.getHeight()));VSTGUI::CColor c;pixels->getColor(c);if(c.alpha<230||(c.red>240&&c.blue>240&&c.green<20))return 0;}
    return !png.empty();
}
extern "C" __declspec(dllexport) int __cdecl JerzyResetMXTransformForTest(Steinberg::IPlugView* view){
    auto*editor=dynamic_cast<JerzyAudio::MXDelayEditor*>(view);if(!editor||!editor->getFrame())return 0;editor->getFrame()->setTransform(VSTGUI::CGraphicsTransform());return 1;
}
