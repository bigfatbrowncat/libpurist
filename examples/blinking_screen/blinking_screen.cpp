#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include <memory>
#define SK_GANESH
#define SK_GL
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <include/gpu/ganesh/gl/GrGLAssembleInterface.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <include/gpu/ganesh/gl/egl/GrGLMakeEGLInterface.h>
#include <include/gpu/ganesh/gl/GrGLDirectContext.h>
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

//#  include <GL/gl.h>
#endif

#include <purist/platform/Platform.h>

#include <map>
#include <cassert>

class SkiaEGLOverlay {
private:
	sk_sp<GrDirectContext> sContext = nullptr;
	sk_sp<SkSurface> sSurface = nullptr;

public:
	SkiaEGLOverlay() {

	}

	~SkiaEGLOverlay() {
		sSurface = nullptr;
		sContext = nullptr;
	}

	const sk_sp<SkSurface> getSkiaSurface() {
		return sSurface;
	}

	const sk_sp<GrDirectContext> getSkiaContext() {
		return sContext;
	}

	void updateBuffer(uint32_t w, uint32_t h) {
		if (sSurface == nullptr || sSurface->width() < w || sSurface->height() < h) {
			auto interface = GrGLInterfaces::MakeEGL();
			sContext = GrDirectContexts::MakeGL(interface);

			GrGLFramebufferInfo framebufferInfo;

			// Wrap the frame buffer object attached to the screen in a Skia render target so Skia can
			// render to it
			GrGLint buffer;
			glGetIntegerv(/*interface,*/ GR_GL_FRAMEBUFFER_BINDING, &buffer);

			// We are always using OpenGL and we use RGBA8 internal format for both RGBA and BGRA configs in OpenGL.
			//(replace line below with this one to enable correct color spaces) framebufferInfo.fFormat = GL_SRGB8_ALPHA8;
			framebufferInfo.fFormat = GR_GL_RGBA8;
			framebufferInfo.fFBOID = (GrGLuint) buffer;

			SkColorType colorType = kRGBA_8888_SkColorType;
			GrBackendRenderTarget backendRenderTarget = GrBackendRenderTargets::MakeGL(w, h,
				0, // sample count
				0, // stencil bits
				framebufferInfo);

			//(replace line below with this one to enable correct color spaces) sSurface = SkSurfaces::WrapBackendRenderTarget(sContext, backendRenderTarget, kBottomLeft_GrSurfaceOrigin, colorType, SkColorSpace::MakeSRGB(), nullptr).release();
			GrRecordingContext* recContext = sContext.get();//.get());
			sSurface = SkSurfaces::WrapBackendRenderTarget(recContext,
				backendRenderTarget,
				kBottomLeft_GrSurfaceOrigin,
				colorType,
				nullptr,
				nullptr);

			if (sSurface == nullptr) {
				throw std::runtime_error("Can not create Skia surface.");
			}
			
			//sContexts[target] = sContext;
			//sSurfaces[target] = sSurface;

		}
	}
};

class ColoredScreenDisplayContents : public purist::platform::DisplayContents {
public:
	uint8_t r, g, b;
	bool r_up, g_up, b_up;
	
	// std::map<std::shared_ptr<purist::platform::TargetSurface>, sk_sp<GrDirectContext>> sContexts;
	// std::map<std::shared_ptr<purist::platform::TargetSurface>, sk_sp<SkSurface>> sSurfaces;
		// sk_sp<GrDirectContext> sContext = nullptr;
		// sk_sp<SkSurface> sSurface = nullptr;
	std::shared_ptr<SkiaEGLOverlay> skiaOverlay;

	ColoredScreenDisplayContents(std::shared_ptr<SkiaEGLOverlay> skiaOverlay) : skiaOverlay(skiaOverlay) {

	}

   /*
    * A short helper function to compute a changing color value. No need to
    * understand it.
    */

    static uint8_t next_color(bool *up, uint8_t cur, unsigned int mod)
    {
        uint8_t next;

        next = cur + (*up ? 1 : -1) * (rand() % mod);
        if ((*up && next < cur) || (!*up && next > cur)) {
            *up = !*up;
            next = cur;
        }

        return next;
    }

	typedef enum class DisplayOrientation {
		HORIZONTAL,
		LEFT_VERTICAL
	} DisplayOrientation;

    void drawIntoBuffer(std::shared_ptr<purist::platform::Display> display, std::shared_ptr<purist::platform::TargetSurface> target) override {
		//DisplayOrientation orientation = DisplayOrientation::HORIZONTAL;// DisplayOrientation::LEFT_VERTICAL;
		
		int w = target->getWidth(), h = target->getHeight();

		// sk_sp<GrDirectContext> sContext = nullptr;
		// sk_sp<SkSurface> sSurface = nullptr;
		// if (sContexts.find(target) == sContexts.end()) {
		// } else {
		// 	sContext = sContexts[target];
		// 	sSurface = sSurfaces[target];
		// 	assert(sSurface->width() == w && sSurface->height() == h);
		// }
		
       	r = next_color(&r_up, r, 20);
        g = next_color(&g_up, g, 10);
        b = next_color(&b_up, b, 5);

		//if (h == 1280) return;

		if (!target->getMappedBuffer()) {
			
			//glClearColor(1.0f/256*r, 1.0f/256*g, 1.0f/256*b, 1.0f);
			//glClear(GL_COLOR_BUFFER_BIT);
			skiaOverlay->updateBuffer(w, h);
			auto sSurface = skiaOverlay->getSkiaSurface();

			auto color = SkColor4f({ 
				1.0f/256 * r,
				1.0f/256 * g,
				1.0f/256 * b,				
				1.0f });
			auto paint = SkPaint(color);

			auto color2 = SkColor4f({ 
				1.0f - 1.0f/256 * r,
				1.0f - 1.0f/256 * g,
				1.0f - 1.0f/256 * b,				
				1.0f });
			auto paint2 = SkPaint(color2);

			auto* canvas = sSurface->getCanvas();
			//canvas->save();
			// if (orientation == DisplayOrientation::LEFT_VERTICAL) {
			// 	canvas->translate(w, 0);
			// 	canvas->rotate(90);
			// }
			canvas->clear(color);

			SkRect rect = SkRect::MakeLTRB(int(w / 3), int(h / 3), int(2 * w / 3), int(2 * h / 3));
			canvas->drawRect(rect, paint2);
			
			//canvas->restore();
			skiaOverlay->getSkiaContext()->flushAndSubmit();

		} else {

			unsigned int j, k, off;

			uint32_t h = target->getHeight();
			uint32_t w = target->getWidth(); 
			uint32_t s = target->getStride(); 
			uint32_t c = (r << 16) | (g << 8) | b;
			for (j = 0; j < h; ++j) {
				for (k = 0; k < w; ++k) {
					off = s * j + k * 4;
					*(uint32_t*)&(target->getMappedBuffer()[off]) = c;
				}
			}
		}
    }
};

class ColoredScreenDisplayContentsFactory : public purist::platform::DisplayContentsFactory {
private:
	std::shared_ptr<SkiaEGLOverlay> skiaOverlay;
public:
	ColoredScreenDisplayContentsFactory() {
		skiaOverlay = std::make_shared<SkiaEGLOverlay>();
	}
	std::shared_ptr<purist::platform::DisplayContents> createDisplayContents(purist::platform::Display& display) {
		
		auto contents = std::make_shared<ColoredScreenDisplayContents>(skiaOverlay);
		contents->r = rand() % 0xff;
		contents->g = rand() % 0xff;
		contents->b = rand() % 0xff;
		contents->r_up = contents->g_up = contents->b_up = true;
		return contents;
	}

};


int main(int argc, char **argv)
{
	try {
		bool enableOpenGL = true;

		purist::platform::Platform purist(enableOpenGL);
		auto contentsFactory = std::make_shared<ColoredScreenDisplayContentsFactory>();
		
		purist.run(contentsFactory);

		return 0;
	
	} catch (const purist::platform::errcode_exception& ex) {
		fprintf(stderr, "%s\n", ex.what());
		return ex.errcode;
	}
}
