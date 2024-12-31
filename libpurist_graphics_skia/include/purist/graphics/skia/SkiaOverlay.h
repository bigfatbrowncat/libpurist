#pragma once

#define SK_GANESH
#define SK_GL

#include <Resource.h>

#include <include/gpu/ganesh/gl/GrGLDirectContext.h>
#include <include/core/SkSurface.h>
#include <include/core/SkFontMgr.h>

#include <vector>
#include <list>

namespace purist::graphics::skia {

class SkiaEGLOverlay;
class SkiaRasterOverlay;

class SkiaOverlay {
private:
	sk_sp<SkFontMgr> fontMgr;
	std::vector<sk_sp<SkData>> dataVec;
public:
	SkiaOverlay();
	virtual ~SkiaOverlay() = default;

	virtual const sk_sp<SkSurface> getSkiaSurface() const = 0;
	virtual const sk_sp<GrDirectContext> getSkiaContext() const = 0;

	virtual SkiaEGLOverlay* asEGLOverlay() = 0;
	virtual SkiaRasterOverlay* asRasterOverlay() = 0;

	void createFontMgr(const std::vector<Resource>& res);
	sk_sp<SkFontMgr> getFontMgr() const { return fontMgr; }
	sk_sp<SkTypeface> getTypefaceByName(const std::string& name) const;
	std::list<SkString> getAllFontFamilyNames() const;
	sk_sp<SkTypeface> getTypefaceForCharacter(SkUnichar unichar, const std::vector<SkString>& fontFamilies, const SkFontStyle& style) const;

};

}