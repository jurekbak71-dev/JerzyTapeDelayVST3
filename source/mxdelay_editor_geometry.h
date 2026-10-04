#pragma once
#include <algorithm>
#include <cmath>
namespace JerzyAudio {
struct MXDelayEditorGeometry {
    static constexpr double width=1280.0,height=720.0;
    static constexpr double minZoom=0.60,maxZoom=1.60;
    static double dpi(double scale){return std::isfinite(scale)&&scale>0.0?scale:1.0;}
    static double zoomForWidth(double pixels,double scale){
        return std::clamp(pixels/(width*dpi(scale)),minZoom,maxZoom);
    }
    static int pixelWidth(double zoom,double scale){return static_cast<int>(std::lround(width*zoom*dpi(scale)));}
    static int pixelHeight(double zoom,double scale){return static_cast<int>(std::lround(height*zoom*dpi(scale)));}
};
}
