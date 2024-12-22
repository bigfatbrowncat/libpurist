#pragma once

#include "SkiaEGLOverlay.h"

#include <purist/graphics/interfaces.h>

namespace purist::graphics::skia {

class DisplayContentsSkia : public DisplayContents {
private:
	std::shared_ptr<SkiaOverlay> skiaOverlay;
public:
    void setSkiaOverlay(std::shared_ptr<SkiaOverlay> skiaOverlay) {
        this->skiaOverlay = skiaOverlay;
    }
    std::shared_ptr<SkiaOverlay> getSkiaOverlay() const { return skiaOverlay; }

    virtual ~DisplayContentsSkia() = default;

    void drawIntoBuffer(std::shared_ptr<Display> display, std::shared_ptr<TargetSurface> target) override {
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

    virtual void drawIntoSurface(std::shared_ptr<Display> display, sk_sp<SkSurface> surface) = 0;

};

}
