#include "SkiaRasterOverlay.h"

#include <include/core/SkSurface.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>

#include <string>
#include <iostream>

namespace purist::graphics::skia {

SkiaRasterOverlay::~SkiaRasterOverlay() {
    sSurface = nullptr;
    sContext = nullptr;
}

const sk_sp<SkSurface> SkiaRasterOverlay::getSkiaSurface() const {
    return sSurface;
}

const sk_sp<GrDirectContext> SkiaRasterOverlay::getSkiaContext() const {
    return nullptr;
}

void SkiaRasterOverlay::updateBuffer(uint32_t w, uint32_t h, void* pixels) {
    // The surface updates each frame, but for a raster backend 
    // it doesn't look like a serious performance issue.

    SkImageInfo imageInfo = SkImageInfo::MakeN32Premul(w, h);
    size_t rowBytes = imageInfo.minRowBytes();

    sSurface = SkSurfaces::WrapPixels(imageInfo, pixels, rowBytes);

    if (sSurface == nullptr) {
        throw std::runtime_error("Can not create Skia surface.");
    }
}

}