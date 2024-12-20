#define SK_GANESH
#define SK_GL

#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <include/gpu/ganesh/gl/GrGLAssembleInterface.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <include/gpu/ganesh/gl/egl/GrGLMakeEGLInterface.h>
#include <include/gpu/ganesh/gl/GrGLDirectContext.h>
#include <include/core/SkSurface.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMgr.h>
#include <include/core/SkColorSpace.h>
#include <include/core/SkSurface.h>

#include <src/gpu/ganesh/gl/GrGLDefines.h>
#include <src/gpu/ganesh/gl/GrGLUtil.h>

#ifdef __APPLE__
#  include "include/ports/SkFontMgr_mac_ct.h"
#else
#  include "include/ports/SkFontMgr_fontconfig.h"
#endif

namespace purist::graphics::skia {

class SkiaEGLOverlay;
class SkiaRasterOverlay;

class SkiaOverlay {
public:
	SkiaOverlay() = default;
	virtual ~SkiaOverlay() = default;

	virtual const sk_sp<SkSurface> getSkiaSurface() const = 0;
	virtual const sk_sp<GrDirectContext> getSkiaContext() const = 0;

	virtual SkiaEGLOverlay* asEGLOverlay() = 0;
	virtual SkiaRasterOverlay* asRasterOverlay() = 0;
};


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

	void updateBuffer(uint32_t w, uint32_t h);
};


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