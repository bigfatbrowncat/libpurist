#pragma once

#include "DisplayContentsSkia.h"

#include <purist/graphics/interfaces.h>

namespace purist::graphics::skia {

class DisplayContentsSkiaFactory : public DisplayContentsFactory {
private:
	std::shared_ptr<SkiaOverlay> skiaOverlay;

public:
    DisplayContentsSkiaFactory(bool enableOpenGL) {
        if (enableOpenGL) {
			skiaOverlay = std::make_shared<purist::graphics::skia::SkiaEGLOverlay>();
		} else {
			skiaOverlay = std::make_shared<purist::graphics::skia::SkiaRasterOverlay>();
		}
    }
    virtual ~DisplayContentsSkiaFactory() = default;

    std::shared_ptr<DisplayContents> createDisplayContents(Display& display) override {
        std::shared_ptr<DisplayContentsSkia> res = createDisplayContentsSkia(display);
        res->setSkiaOverlay(skiaOverlay);
        return res;
    }

    virtual std::shared_ptr<DisplayContentsSkia> createDisplayContentsSkia(Display& display) = 0;
};

}