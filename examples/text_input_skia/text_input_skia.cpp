// libpurist headers
#include <purist/graphics/skia/TextInput.h>
#include <purist/graphics/skia/DisplayContentsSkia.h>
#include <purist/graphics/Display.h>
#include <purist/graphics/Mode.h>
#include <purist/input/KeyboardHandler.h>
#include <purist/Platform.h>

#include <Resource.h>

// Skia headers
#include <include/core/SkCanvas.h>
#include <include/core/SkSurface.h>
#include <include/core/SkImage.h>
#include <include/core/SkPaint.h>
#include <include/core/SkFont.h>
#include <include/core/SkData.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkColor.h>

// std headers
#include <map>
#include <memory>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

namespace p = purist;
namespace pg = purist::graphics;
namespace pi = purist::input;
namespace pgs = purist::graphics::skia;

class ColoredScreenDisplayContents : public pgs::SkiaDisplayContentsHandler, public pi::KeyboardHandler {
public:
	std::weak_ptr<p::Platform> platform;

	uint8_t r, g, b;
	bool r_up, g_up, b_up;

	uint32_t cursor_phase = 0;
	uint32_t hue_phase = 0;
	uint32_t value_phase = 0;
	//sk_sp<SkTypeface> typeface;
	pgs::TextInput textInput;

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
		
		//i = (i + 1) % 5;

		//if (i == 0) top = !top;
		
		// Looking for the highest resolution at 30 fps
		
		std::list<std::shared_ptr<pg::Mode>>::const_iterator res = modes.begin();

		int index = 0;
		for(auto mode = modes.begin(); mode != modes.end(); mode++) {
			//std::cout << "Mode " << index << ": " << (*mode)->getWidth() << "x" << (*mode)->getHeight() << "@" << (*mode)->getFreq() << std::endl;
			index++;
			if ((*mode)->getFreq() == 30 && (*mode)->getWidth() > (*res)->getWidth()) {
				res = mode;
			}
		}

		if (skiaOverlay->getFontMgr() == nullptr) {
			skiaOverlay->createFontMgr({
				LOAD_RESOURCE(fonts_noto_sans_NotoSans_Regular_ttf),
				LOAD_RESOURCE(fonts_noto_sans_NotoSansHebrew_Regular_ttf)
			});
		}

		return res;
	}

	pgs::DisplayOrientation chooseOrientation(std::shared_ptr<pg::Display> display, std::shared_ptr<pgs::SkiaOverlay> skiaOverlay) override {
		if (display->getMode().getHeight() > display->getMode().getWidth()) {
			// We are assumming that all the displays are horizontally oriented.
			// So if some of them have height > width, let's rotate it
			return pgs::DisplayOrientation::LEFT_VERTICAL;
		} else {
			return pgs::DisplayOrientation::HORIZONTAL;
		}
	}

	static float squareFunctionHarmonics(float x, uint32_t count) {
		float y = 0;
		for (uint32_t i = 0; i < count; i++) {
			auto n = 2 * i - 1;
			y += 1.0/n * sin(n * 2 * M_PI * x);
		}
		return y;
	}

    void drawIntoSurface(std::shared_ptr<pg::Display> display, std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, int width, int height, SkCanvas& canvas) override {
		SkScalar w = width, h = height;

		uint32_t hue_period = 1597;  // prime
		uint32_t value_period = 4211; // prime

		hue_phase = (hue_phase + 1) % hue_period;
		value_phase = (value_phase + 1) % value_period;
		
		float float_hue_phase = (float)hue_phase / hue_period;
		float float_value_phase = (float)value_phase / value_period;

		SkScalar hue = float_hue_phase * 360.0;
		SkScalar saturation = 0.6;
		SkScalar value = 0.3 * squareFunctionHarmonics(float_value_phase, 5) + 0.5;
		
		const SkScalar color_hsv[] { hue, saturation, 0.9f * value };
		auto color = SkColor4f::FromColor(SkHSVToColor(255, color_hsv));
		
		auto hue2 = hue + 180; if (hue2 > 360) hue2 -= 360;
		const SkScalar color2_hsv[] { hue2, saturation, 0.9f * (1.0f - value) };
		auto color2 = SkColor4f::FromColor(SkHSVToColor(255, color2_hsv));

		auto paint = SkPaint(color);
		auto paint2 = SkPaint(color2);


		canvas.clear(color);

		SkRect rect = SkRect::MakeLTRB(
			w * 0.9 / 5, h / 5, 
			w * 4.1 / 5, 4 * h / 5);
		canvas.drawRect(rect, paint2);

		int sz = (int)(0.8 * h / sqrt(4 * fmax(1, textInput.getText().countChar32() - 2)));
		textInput.setFontSize(sz);

    	textInput.setWidth(3 * w / 5);
		textInput.setHeight(h);
		textInput.setLeft(w / 5);
		textInput.setTop(h / 2);
		textInput.setPaint(paint);

		textInput.drawIntoSurface(display, skiaOverlay, canvas);
    }

    void onCharacter(pi::Keyboard& kbd, char32_t charCode, pi::Modifiers mods, pi::Leds leds) override { 
		textInput.onCharacter(kbd, charCode);
	}
    void onKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat) override { 
		if (keysym == XKB_KEY_Escape) {
			platform.lock()->stop();
		} else {
			textInput.onKeyPress(kbd, keysym, mods, leds, repeat);
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
