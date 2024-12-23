#include <purist/graphics/skia/DisplayContentsSkia.h>

#include "SkiaRasterOverlay.h"
#include "SkiaEGLOverlay.h"

namespace purist::graphics::skia {

void DisplayContentsSkia::setSkiaOverlay(std::shared_ptr<SkiaOverlay> skiaOverlay) {
    this->skiaOverlay = skiaOverlay;
}
std::shared_ptr<SkiaOverlay> DisplayContentsSkia::getSkiaOverlay() const { return skiaOverlay; }

void DisplayContentsSkia::drawIntoBuffer(std::shared_ptr<Display> display, std::shared_ptr<TargetSurface> target) {
    int tw = target->getWidth(), th = target->getHeight();

    if (!target->getMappedBuffer()) {
        auto* eglOverlay = skiaOverlay->asEGLOverlay();
        eglOverlay->updateBuffer(tw, th);
    } else {
        auto* rasterOverlay = skiaOverlay->asRasterOverlay();
        rasterOverlay->updateBuffer(tw, th, target->getMappedBuffer());
    }

    auto sSurface = skiaOverlay->getSkiaSurface();

	//w = sSurface->width();
	//h = sSurface->height();

    uint32_t w = tw;//display->getWidth();
    uint32_t h = th;//display->getHeight();
		
    if (h > w) {
        // We are assumming that the display is horizontally oriented.
        // So if some of them has height > width, let's rotate it
        orientation = DisplayOrientation::LEFT_VERTICAL;
    }

	auto* canvas = sSurface->getCanvas();
    canvas->save();

    // With EGL backend SkSurface can be bigger than the Display
    // (because the surface will have the dimensions of the biggest display connected to the system)
    // To align the display with the surface, we need this translation
    canvas->translate(0, sSurface->height() - display->getHeight());
    
    if (orientation == DisplayOrientation::LEFT_VERTICAL) {
        canvas->translate(w, 0);
        canvas->rotate(90);
        std::swap(w, h);
    } else if (orientation == DisplayOrientation::RIGHT_VERTICAL) {
        canvas->translate(0, h);
        canvas->rotate(-90);
        std::swap(w, h);
    }
    
    drawIntoSurface(display, w, h, *canvas);

    canvas->restore();

    
    auto context = skiaOverlay->getSkiaContext();
    if (context != nullptr) {
        context->flushAndSubmit();
    }
}

}