// libpurist headers
#include <purist/graphics/skia/DisplayContentsSkia.h>
#include <purist/graphics/Display.h>
#include <purist/graphics/Mode.h>
#include <purist/input/interfaces.h>
#include <purist/Platform.h>

// Skia headers
#include <include/core/SkSurface.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>

#include <xkbcommon/xkbcommon-keysyms.h>

// std headers
#include <map>
#include <memory>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace p = purist;
namespace pg = purist::graphics;
namespace pi = purist::input;
namespace pgs = purist::graphics::skia;

class ColoredScreenDisplayContents : public pgs::SkiaDisplayContentsHandler, public pi::KeyboardHandler {
public:
	std::weak_ptr<p::Platform> platform;

	uint8_t r, g, b;
	bool r_up, g_up, b_up;

	sk_sp<SkTypeface> typeface;
	std::shared_ptr<SkFont> font;

	std::string letter;

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

	ColoredScreenDisplayContents(std::weak_ptr<p::Platform> platform) : platform(platform) {
		r = rand() % 0xff;
		g = rand() % 0xff;
		b = rand() % 0xff;
		r_up = g_up = b_up = true;
	}
	
	int i = 0;
	bool top = true;
    std::list<std::shared_ptr<pg::Mode>>::const_iterator chooseMode(std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, const std::list<std::shared_ptr<pg::Mode>>& modes) override {
		
		i = (i + 1) % 5;

		if (i == 0) top = !top;

		auto res = modes.begin();
		if (top) {
			auto res = modes.end();  //.begin();
			res--;
		}

		if (typeface == nullptr) {
			typeface = skiaOverlay->getTypeface("sans-serif");
		}
		font = std::make_shared<SkFont>(typeface, (*res)->getHeight() / 4);

		return res;
	}

	pgs::DisplayOrientation chooseOrientation(std::shared_ptr<pg::Display> display, std::shared_ptr<pgs::SkiaOverlay> skiaOverlay) override {
		if (display->getMode().getHeight() > display->getMode().getWidth()) {
			// We are assumming that the display is horizontally oriented.
			// So if some of them has height > width, let's rotate it
			return pgs::DisplayOrientation::LEFT_VERTICAL;
		} else {
			return pgs::DisplayOrientation::HORIZONTAL;
		}
	}

    void drawIntoSurface(std::shared_ptr<pg::Display> display, std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, int width, int height, SkCanvas& canvas) override {
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

		SkRect rect = SkRect::MakeLTRB(w / 5, h / 5, 4 * w / 5, 4 * h / 5);
		canvas.drawRect(rect, paint2);


		//std::string topText { "Top" };
		SkRect letterBounds;
		font->measureText(letter.c_str(), letter.size(), SkTextEncoding::kUTF8, &letterBounds);
		SkFontMetrics letterMetrics;
		font->getMetrics(&letterMetrics);

		canvas.drawString(letter.c_str(), 
				w / 2 - letterBounds.centerX(), //.width() / 2, 
				h / 2 - (letterMetrics.fAscent + letterMetrics.fDescent) / 2, *font, paint);
    }

    void onCharacter(pi::Keyboard& kbd, uint32_t utf8CharCode) override { 
		if (utf8CharCode <= 0x1F || utf8CharCode == 0x7F) {
			std::stringstream ss;
			ss << "0x" << std::setfill ('0') << std::setw(2) << std::hex << utf8CharCode;
			letter = ss.str();
		} else {
			letter = " ";
			letter[0] = (uint8_t)utf8CharCode;
		}
	}
    void onKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat) override { 
		if (keysym == XKB_KEY_Escape) {
			platform.lock()->stop();
		}
	}
    void onKeyRelease(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds) override { }
};


int main(int argc, char **argv)
{
	try {
		bool enableOpenGL = true;

		auto purist = std::make_shared<p::Platform>(enableOpenGL);
		
		auto contents = std::make_shared<ColoredScreenDisplayContents>(purist);
		auto contents_handler_for_skia = std::make_shared<pgs::DisplayContentsHandlerForSkia>(contents, enableOpenGL);
		
		purist->run(contents_handler_for_skia, contents);

		return 0;
	
	} catch (const purist::errcode_exception& ex) {
		fprintf(stderr, "%s\n", ex.what());
		return ex.errcode;
	}
}
