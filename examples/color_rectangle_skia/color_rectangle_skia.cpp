// libpurist headers
#include <purist/Platform.h>
#include <purist/graphics/skia/DisplayContentsSkiaFactory.h>

// Skia headers
#include <include/core/SkSurface.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>

// std headers
#include <map>
#include <memory>
#include <iostream>

namespace pg = purist::graphics;
namespace pgs = purist::graphics::skia;

class ColoredScreenDisplayContents : public pgs::DisplayContentsSkia {
public:
	uint8_t r, g, b;
	bool r_up, g_up, b_up;

	sk_sp<SkTypeface> typeface;
	std::shared_ptr<SkFont> font;

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
	
	int i = 0;
	bool top = true;
    std::list<std::shared_ptr<pg::Mode>>::const_iterator chooseMode(const std::list<std::shared_ptr<pg::Mode>>& modes) override {
		
		i = (i + 1) % 5;

		if (i == 0) top = !top;

		if (top) {
			auto res = modes.end();  //.begin();
			res--;
			return res;
		} else {
			auto res = modes.begin();
			return res;
		}
	}


    void drawIntoSurface(std::shared_ptr<pg::Display> display, int width, int height, SkCanvas& canvas) override {
		SkScalar w = width, h = height;

       	r = next_color(&r_up, r, 20);
        g = next_color(&g_up, g, 10);
        b = next_color(&b_up, b, 5);

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

		canvas.clear(color);

		SkRect rect = SkRect::MakeLTRB(w / 3, h / 3, 2 * w / 3, 2 * h / 3);
		canvas.drawRect(rect, paint2);

		if (typeface == nullptr) {
			typeface = getSkiaOverlay()->getTypeface("sans-serif");
		}
		if (font == nullptr) {
			font = std::make_shared<SkFont>(typeface, 50);
		}

		std::string topText { "Top" };
		SkRect topTextBounds;
		font->measureText(topText.c_str(), topText.size(), SkTextEncoding::kUTF8, &topTextBounds);
		SkFontMetrics topTextMetrics;
		font->getMetrics(&topTextMetrics);

		std::string bottomText { "Bottom" };
		SkRect bottomTextBounds;
		font->measureText(bottomText.c_str(), bottomText.size(), SkTextEncoding::kUTF8, &bottomTextBounds);
		SkFontMetrics bottomTextMetrics;
		font->getMetrics(&bottomTextMetrics);

	
		canvas.drawString(topText.c_str(), 0, 0 - topTextMetrics.fAscent, *font, paint2);
		canvas.drawString(topText.c_str(), w - topTextBounds.fRight, 0 - topTextMetrics.fAscent, *font, paint2);

		canvas.drawString(bottomText.c_str(), 0, h - bottomTextMetrics.fDescent, *font, paint2);
		canvas.drawString(bottomText.c_str(), w - bottomTextBounds.fRight, h - bottomTextMetrics.fDescent, *font, paint2);

    }
};

class ColoredScreenDisplayContentsFactory : public pgs::DisplayContentsSkiaFactory {
public:
	ColoredScreenDisplayContentsFactory(bool enableOpenGL) : DisplayContentsSkiaFactory(enableOpenGL) { }

	std::shared_ptr<pgs::DisplayContentsSkia> createDisplayContentsSkia(pg::Display& display) {
		auto contents = std::make_shared<ColoredScreenDisplayContents>();
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
		auto contentsFactory = std::make_shared<ColoredScreenDisplayContentsFactory>(enableOpenGL);
		
		purist.run(contentsFactory);

		return 0;
	
	} catch (const purist::errcode_exception& ex) {
		fprintf(stderr, "%s\n", ex.what());
		return ex.errcode;
	}
}
