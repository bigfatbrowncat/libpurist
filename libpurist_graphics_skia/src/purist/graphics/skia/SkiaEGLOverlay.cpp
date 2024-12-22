#include <purist/graphics/skia/SkiaEGLOverlay.h>

#include <include/core/SkSurface.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>

#include <string>
#include <iostream>

namespace purist::graphics::skia {

void SkiaOverlay::createFontMgr() {
    fontMgr = SkFontMgr_New_FontConfig(nullptr);
    int families = fontMgr->countFamilies();
    std::cout << "Font families count: " << families << std::endl;
}

SkiaOverlay::SkiaOverlay() {
    createFontMgr();
}

sk_sp<SkTypeface> SkiaOverlay::getTypeface(const std::string& name) const {
  //auto tf = SkFontMgr::RefEmpty()->makeFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0);  //ToolUtils::CreatePortableTypeface("serif", SkFontStyle());
  //auto tf = fontMgr->makeFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0);  //ToolUtils::CreatePortableTypeface("serif", SkFontStyle());
  auto tf = fontMgr->matchFamilyStyle(/*"sans-serif"*/name.c_str(), SkFontStyle());
  if (tf == nullptr) {
    throw std::runtime_error(std::string("No typeface found") + name);
  }
  return tf;
}


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
    if (sSurface == nullptr || sSurface->width() < w || sSurface->height() < h) {
        auto interface = GrGLInterfaces::MakeEGL();
        sContext = GrDirectContexts::MakeGL(interface);

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
        GrBackendRenderTarget backendRenderTarget = GrBackendRenderTargets::MakeGL(w, h,
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