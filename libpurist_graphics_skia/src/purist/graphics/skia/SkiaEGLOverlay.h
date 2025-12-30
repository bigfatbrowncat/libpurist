#pragma once

#include <purist/graphics/skia/SkiaOverlay.h>

namespace purist::graphics::skia {

class SkiaEGLOverlay : public SkiaOverlay {
private:
	sk_sp<GrDirectContext> sContext = nullptr;
	sk_sp<SkSurface> sSurface = nullptr;

public:
	SkiaEGLOverlay() = default;
	~SkiaEGLOverlay();

	const sk_sp<SkSurface> getSkiaSurface() const override;
	const sk_sp<GrDirectContext> getSkiaContext() const override;

	SkiaEGLOverlay* asEGLOverlay() override { return this; }
	SkiaRasterOverlay* asRasterOverlay() override { return nullptr; } 

	bool updateBuffer(uint32_t w, uint32_t h);
};

}