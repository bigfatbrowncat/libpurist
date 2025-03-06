#include "SkiaEGLOverlay.h"

#include <include/core/SkColorSpace.h>
#include <include/core/SkSurface.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/gpu/ganesh/gl/GrGLTypes.h>
#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <include/gpu/ganesh/gl/egl/GrGLMakeEGLInterface.h>
#include <src/gpu/ganesh/gl/GrGLDefines.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>

#include <string>
#include <iostream>
#include <cassert>

namespace purist::graphics::skia {

SkiaEGLOverlay::~SkiaEGLOverlay() {
    sSurface = nullptr;
    sContext = nullptr;
}

const sk_sp<SkSurface> SkiaEGLOverlay::getSkiaSurface() const {
    return sSurface;
}

const sk_sp<GrDirectContext> SkiaEGLOverlay::getSkiaContext() const {
    return sContext;
}

void SkiaEGLOverlay::updateBuffer(uint32_t w, uint32_t h) {
    if (sContext == nullptr || sContext->abandoned()) {
        // TODO When the context is recreated, all SkImage objects become invalid! SO SOME SIGNAL SHOULD BE PASSED TO THE CLIENT!

        auto interface = GrGLInterfaces::MakeEGL();
        sContext = GrDirectContexts::MakeGL(interface);
        
        sSurface = nullptr;
    }

    assert(sSurface->width() > 0 && sSurface->height() > 0);
    if (sSurface == nullptr || (uint32_t)sSurface->width() < w || (uint32_t)sSurface->height() < h) {
        std::cout << "Allocating EGL surface: " << w << "x" << h << std::endl;
        // auto interface = GrGLInterfaces::MakeEGL();
        // sContext = GrDirectContexts::MakeGL(interface);

        GrGLFramebufferInfo framebufferInfo;

        // Wrap the frame buffer object attached to the screen in a Skia render target so Skia can
        // render to it
        GrGLint buffer;
        glGetIntegerv(/*interface,*/ GR_GL_FRAMEBUFFER_BINDING, &buffer);

        // We are always using OpenGL and we use RGBA8 internal format for both RGBA and BGRA configs in OpenGL.
        //(replace line below with this one to enable correct color spaces) framebufferInfo.fFormat = GL_SRGB8_ALPHA8;
        framebufferInfo.fFormat = GR_GL_RGBA8;
        framebufferInfo.fFBOID = (GrGLuint) buffer;

        SkColorType colorType = kRGBA_8888_SkColorType;
        uint32_t mw = w, mh = h;
        if (sSurface != nullptr) {
            mw = (uint32_t)sSurface->width() > w ? sSurface->width() : w;
            mh = (uint32_t)sSurface->height() > h ? sSurface->height() : h;
        }

        GrBackendRenderTarget backendRenderTarget = GrBackendRenderTargets::MakeGL(mw, mh,
            0, // sample count
            0, // stencil bits
            framebufferInfo);

        //(replace line below with this one to enable correct color spaces) sSurface = SkSurfaces::WrapBackendRenderTarget(sContext, backendRenderTarget, kBottomLeft_GrSurfaceOrigin, colorType, SkColorSpace::MakeSRGB(), nullptr).release();
        GrRecordingContext* recContext = sContext.get();//.get());

        sSurface = SkSurfaces::WrapBackendRenderTarget(recContext,
            backendRenderTarget,
            kBottomLeft_GrSurfaceOrigin,
            colorType,
            nullptr,
            nullptr);

        if (sSurface == nullptr) {
            throw std::runtime_error("Can not create Skia surface.");
        }
    }
}

}