#pragma once

#include <purist/graphics/skia/SkiaOverlay.h>

#include <include/gpu/ganesh/GrDirectContext.h>

namespace purist::graphics::skia {

class SkiaRasterOverlay : public SkiaOverlay {
private:
	sk_sp<GrRecordingContext> sContext = nullptr;
	sk_sp<SkSurface> sSurface = nullptr;

public:
	SkiaRasterOverlay() = default;
	~SkiaRasterOverlay();

	const sk_sp<SkSurface> getSkiaSurface() const override;
	const sk_sp<GrDirectContext> getSkiaContext() const override;

	SkiaEGLOverlay* asEGLOverlay() override { return nullptr; }
	SkiaRasterOverlay* asRasterOverlay() override { return this; } 

	void updateBuffer(uint32_t w, uint32_t h, void* pixels);
};

}