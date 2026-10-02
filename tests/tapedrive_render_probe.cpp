// Only compiled with BUILD_TESTING. Render the actual DLL's CFrame through its
// graphics backend so integration tests can inspect pixels, not just hit areas.
#include "../source/tapedrive_editor.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/coffscreencontext.h"
#include "vstgui/lib/cbitmap.h"
#include "vstgui/lib/platform/platformfactory.h"
#include <fstream>
#include <iostream>
extern "C" __declspec(dllexport) int __cdecl JerzyRenderEditorForTest(Steinberg::IPlugView* view,const char* path){
    auto* editor=dynamic_cast<JerzyAudio::TapeDriveEditor*>(view);
    if(!editor || !editor->getFrame())return 0;
    auto* frame=editor->getFrame();
    const auto rect=frame->getViewSize();
    auto context=VSTGUI::COffscreenContext::create({rect.getWidth(),rect.getHeight()});
    if(!context)return 0;
    context->beginDraw();
    context->setFillColor(VSTGUI::CColor(255,0,255));
    context->drawRect(rect,VSTGUI::kDrawFilled);
    frame->drawRect(context,rect);
    context->endDraw();
    auto* bitmap=context->getBitmap();
    const auto png=VSTGUI::getPlatformFactory().createBitmapMemoryPNGRepresentation(bitmap->getPlatformBitmap());
    std::ofstream image(path,std::ios::binary);
    image.write(reinterpret_cast<const char*>(png.data()),png.size());
    auto pixels=VSTGUI::owned(VSTGUI::CBitmapPixelAccess::create(bitmap));
    if(!pixels)return 0;
    // Wood near all four edges must be opaque and painted; the former blank
    // right/bottom strips (or an unscaled bitmap) must fail this test.
    for(double y:{.05,.95})for(double x:{.035,.965}){
        int painted=0;
        for(int j=-3;j<=3;++j)for(int i=-3;i<=3;++i){
            pixels->setPosition(static_cast<uint32_t>(x*rect.getWidth()+i),static_cast<uint32_t>(y*rect.getHeight()+j));
            VSTGUI::CColor c;pixels->getColor(c);
            if(c.alpha>240 && c.red+c.green+c.blue>45 && !(c.red>240&&c.blue>240&&c.green<10))++painted;
        }
        std::cout<<"Rendered edge patch "<<x<<','<<y<<": "<<painted<<"/49 painted pixels\n";
        if(painted<40)return 0;
    }
    return !png.empty();
}
