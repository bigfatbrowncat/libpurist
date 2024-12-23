#pragma once

#include <purist/graphics/skia/SkiaOverlay.h>
#include <purist/graphics/interfaces.h>

namespace purist::graphics::skia {

typedef enum class DisplayOrientation {
    HORIZONTAL,
    LEFT_VERTICAL
} DisplayOrientation;


class DisplayContentsSkia : public DisplayContents {
private:
	std::shared_ptr<SkiaOverlay> skiaOverlay;
    DisplayOrientation orientation;
public:
    void setSkiaOverlay(std::shared_ptr<SkiaOverlay> skiaOverlay);
    std::shared_ptr<SkiaOverlay> getSkiaOverlay() const;

    virtual ~DisplayContentsSkia() = default;

    void drawIntoBuffer(std::shared_ptr<Display> display, std::shared_ptr<TargetSurface> target) override;

    virtual void drawIntoSurface(std::shared_ptr<purist::graphics::Display> display, int width, int height, SkCanvas& canvas) = 0;

};

}
