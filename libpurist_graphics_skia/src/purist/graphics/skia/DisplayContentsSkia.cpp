#include <purist/graphics/skia/DisplayContentsSkia.h>

#include "SkiaRasterOverlay.h"
#include "SkiaEGLOverlay.h"

namespace purist::graphics::skia {

void DisplayContentsSkia::setSkiaOverlay(std::shared_ptr<SkiaOverlay> skiaOverlay) {
    this->skiaOverlay = skiaOverlay;
}
std::shared_ptr<SkiaOverlay> DisplayContentsSkia::getSkiaOverlay() const { return skiaOverlay; }

void DisplayContentsSkia::drawIntoBuffer(std::shared_ptr<Display> display, std::shared_ptr<TargetSurface> target) {
    int w = target->getWidth(), h = target->getHeight();

    if (!target->getMappedBuffer()) {
        auto* eglOverlay = skiaOverlay->asEGLOverlay();
        eglOverlay->updateBuffer(w, h);
    } else {
        auto* rasterOverlay = skiaOverlay->asRasterOverlay();
        rasterOverlay->updateBuffer(w, h, target->getMappedBuffer());
    }

    auto sSurface = skiaOverlay->getSkiaSurface();
    
    drawIntoSurface(display, sSurface);
    
    auto context = skiaOverlay->getSkiaContext();
    if (context != nullptr) {
        context->flushAndSubmit();
    }
}

}