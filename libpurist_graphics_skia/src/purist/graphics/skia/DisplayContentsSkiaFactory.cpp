#include <purist/graphics/skia/DisplayContentsSkiaFactory.h>

#include "SkiaRasterOverlay.h"
#include "SkiaEGLOverlay.h"

namespace purist::graphics::skia {

DisplayContentsSkiaFactory::DisplayContentsSkiaFactory(bool enableOpenGL) {
    if (enableOpenGL) {
        skiaOverlay = std::make_shared<purist::graphics::skia::SkiaEGLOverlay>();
    } else {
        skiaOverlay = std::make_shared<purist::graphics::skia::SkiaRasterOverlay>();
    }
}

std::shared_ptr<DisplayContents> DisplayContentsSkiaFactory::createDisplayContents(Display& display) {
    std::shared_ptr<DisplayContentsSkia> res = createDisplayContentsSkia(display);
    res->setSkiaOverlay(skiaOverlay);
    return res;
}

}