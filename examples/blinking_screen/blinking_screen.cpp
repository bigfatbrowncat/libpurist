#include <purist/Platform.h>
#include <purist/graphics/skia/SkiaEGLOverlay.h>

#include <map>
#include <memory>
//#include <cassert>


class ColoredScreenDisplayContents : public purist::graphics::DisplayContents {
public:
	uint8_t r, g, b;
	bool r_up, g_up, b_up;
	
	std::shared_ptr<purist::graphics::skia::SkiaEGLOverlay> skiaOverlay;

	ColoredScreenDisplayContents(std::shared_ptr<purist::graphics::skia::SkiaEGLOverlay> skiaOverlay) : skiaOverlay(skiaOverlay) {

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

    void drawIntoBuffer(std::shared_ptr<purist::graphics::Display> display, std::shared_ptr<purist::graphics::TargetSurface> target) override {
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

class ColoredScreenDisplayContentsFactory : public purist::graphics::DisplayContentsFactory {
private:
	std::shared_ptr<purist::graphics::skia::SkiaEGLOverlay> skiaOverlay;
public:
	ColoredScreenDisplayContentsFactory() {
		skiaOverlay = std::make_shared<purist::graphics::skia::SkiaEGLOverlay>();
	}
	std::shared_ptr<purist::graphics::DisplayContents> createDisplayContents(purist::graphics::Display& display) {
		
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

		purist::Platform purist(enableOpenGL);
		auto contentsFactory = std::make_shared<ColoredScreenDisplayContentsFactory>();
		
		purist.run(contentsFactory);

		return 0;
	
	} catch (const purist::errcode_exception& ex) {
		fprintf(stderr, "%s\n", ex.what());
		return ex.errcode;
	}
}
