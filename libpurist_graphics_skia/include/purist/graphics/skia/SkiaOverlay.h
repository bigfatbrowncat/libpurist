#pragma once

#define SK_GANESH
#define SK_GL

#include <include/gpu/ganesh/gl/GrGLDirectContext.h>
#include <include/core/SkSurface.h>
#include <include/core/SkFontMgr.h>

namespace purist::graphics::skia {

class SkiaEGLOverlay;
class SkiaRasterOverlay;

class SkiaOverlay {
private:
	sk_sp<SkFontMgr> fontMgr;
	void createFontMgr();
  
public:
	SkiaOverlay();
	virtual ~SkiaOverlay() = default;

	virtual const sk_sp<SkSurface> getSkiaSurface() const = 0;
	virtual const sk_sp<GrDirectContext> getSkiaContext() const = 0;

	virtual SkiaEGLOverlay* asEGLOverlay() = 0;
	virtual SkiaRasterOverlay* asRasterOverlay() = 0;

	sk_sp<SkTypeface> getTypeface(const std::string& name) const;
};

}