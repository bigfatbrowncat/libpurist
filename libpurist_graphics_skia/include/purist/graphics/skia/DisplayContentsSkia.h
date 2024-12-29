#pragma once

#include <purist/graphics/skia/SkiaOverlay.h>
#include <purist/graphics/DisplayContentsHandler.h>

#include <memory>

namespace purist::graphics::skia {

typedef enum class DisplayOrientation {
    HORIZONTAL,
    LEFT_VERTICAL,
    RIGHT_VERTICAL
} DisplayOrientation;


class SkiaDisplayContentsHandler {
public:
    virtual ~SkiaDisplayContentsHandler() = default;

    virtual std::list<std::shared_ptr<Mode>>::const_iterator chooseMode(
        std::shared_ptr<SkiaOverlay> skiaOverlay,
        const std::list<std::shared_ptr<Mode>>& modes) { return modes.begin(); }

    virtual DisplayOrientation chooseOrientation(
        std::shared_ptr<purist::graphics::Display> display,
        std::shared_ptr<SkiaOverlay> skiaOverlay) { return DisplayOrientation::HORIZONTAL; }

    virtual void drawIntoSurface(
        std::shared_ptr<purist::graphics::Display> display, 
        std::shared_ptr<SkiaOverlay> skiaOverlay,
        int width, int height, SkCanvas& canvas) = 0;
};

class DisplayContentsHandlerForSkia final : public DisplayContentsHandler {
private:
	std::shared_ptr<SkiaOverlay> skiaOverlay;
    std::shared_ptr<SkiaDisplayContentsHandler> userContentsHandler;
public:
    DisplayContentsHandlerForSkia(std::shared_ptr<SkiaDisplayContentsHandler> userContentsHandler, bool enableOpenGL);
    virtual ~DisplayContentsHandlerForSkia() = default;

    void setSkiaOverlay(std::shared_ptr<SkiaOverlay> skiaOverlay);
    std::shared_ptr<SkiaOverlay> getSkiaOverlay() const;

    void drawIntoBuffer(std::shared_ptr<Display> display, std::shared_ptr<TargetSurface> target) override;
    std::list<std::shared_ptr<Mode>>::const_iterator chooseMode(const std::list<std::shared_ptr<Mode>>& modes) override;

};

}
