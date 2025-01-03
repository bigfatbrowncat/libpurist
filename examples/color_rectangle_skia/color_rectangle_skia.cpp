// libpurist headers
#include "modules/skparagraph/include/ParagraphBuilder.h"
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

//#include <modules/skparagraph/include/ParagraphBuilder.h>
#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skparagraph/include/FontCollection.h>
#include <modules/skparagraph/include/TextStyle.h>

//#include <icu/source/common/unicode/urename.h>
#define U_DISABLE_VERSION_SUFFIX	1
#include <icu/source/common/unicode/utf8.h>


//#include <unicode/utf8.h>
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

	//sk_sp<SkTypeface> typeface;

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

		auto cursor_color = SkColor4f({ 
			1.0f,
			1.0f,
			1.0f,
			0.5f });
		auto cursor_paint = SkPaint(cursor_color);

		canvas.clear(color);

		SkRect rect = SkRect::MakeLTRB(w / 5, h / 5, 4 * w / 5, 4 * h / 5);
		canvas.drawRect(rect, paint2);

		int sz = (int)(h / sqrt(4 * fmax(1, letter.size())));
		//auto font = std::make_shared<SkFont>(typeface, sz);

		// SkRect letterBounds;
		// font->measureText(letter.c_str(), letter.size(), SkTextEncoding::kUTF8, &letterBounds);
		// SkFontMetrics letterMetrics;
		// font->getMetrics(&letterMetrics);

		std::vector<SkString> fontFamilies { SkString("Noto Sans"), SkString("Noto Sans Hebrew") };

		skia::textlayout::TextStyle style;
        style.setBackgroundColor(paint2);
        style.setForegroundColor(paint);
        style.setFontFamilies(fontFamilies);
        style.setFontSize(sz);
		
		skia::textlayout::ParagraphStyle paraStyle;
		paraStyle.setHeight(sz);
        paraStyle.setTextStyle(style);
        paraStyle.setTextAlign(skia::textlayout::TextAlign::kCenter);
		auto fontCollection = sk_make_sp<skia::textlayout::FontCollection>();
        fontCollection->setDefaultFontManager(skiaOverlay->getFontMgr());
		skia::textlayout::ParagraphBuilderImpl builder(paraStyle, fontCollection);
		builder.addText(letter.c_str(), letter.size());
		auto paragraph = builder.Build();
		paragraph->layout(3 * w / 5);
		auto text_center_x = w / 5;
		auto text_center_y = h / 2 - paragraph->getHeight() / 2;

		if (letter.size() > 0) {
			int text_len = 0;
			for (int32_t prev_pos = letter.size(); prev_pos > 0; text_len++) { 
				U8_BACK_1((uint8_t*)letter.c_str(), 0, prev_pos);
			}
			//std::cout << letter.size() << " -> " << prev_pos << std::endl;

			std::vector<skia::textlayout::TextBox> boxes = paragraph->getRectsForRange(text_len - 1, text_len,//prev_pos, letter.size(),
													skia::textlayout::RectHeightStyle::kMax,
													skia::textlayout::RectWidthStyle::kTight);
			paragraph->paint(&canvas, text_center_x, text_center_y);

			if (boxes.size() > 0) {
				auto& box = *boxes.begin();
				auto rect = box.rect;
				rect.offset(text_center_x, text_center_y);
				canvas.drawRect(rect, cursor_paint);
			}
		}


		// canvas.drawString(letter.c_str(), 
		// 		w / 2 - letterBounds.centerX(), //.width() / 2, 
		// 		h / 2 - (letterMetrics.fAscent + letterMetrics.fDescent) / 2, *font, paint);
    }

    void onCharacter(pi::Keyboard& kbd, char utf8CharCode[4]) override { 
		if (utf8CharCode[0] <= 0x1F || utf8CharCode[0] == 0x7F) {
			std::stringstream ss;
			uint32_t code = reinterpret_cast<uint8_t&>(utf8CharCode[0]);
			ss << "0x" << std::setfill ('0') << std::setw(2) << std::hex << code;
			//letter = ss.str();
		} else {
			letter += utf8CharCode;
		}
	}
    void onKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat) override { 
		if (keysym == XKB_KEY_Escape) {
			platform.lock()->stop();

		} else if (keysym == XKB_KEY_BackSpace) {
			if (letter.size() > 0) {
				int32_t pos = letter.size();
				U8_BACK_1((uint8_t*)letter.c_str(), 0, pos);

				letter = letter.substr(0, pos);
			}
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
