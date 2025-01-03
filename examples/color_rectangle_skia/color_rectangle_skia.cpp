// libpurist headers
#include "include/core/SkColor.h"
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
#define U_DISABLE_RENAMING			1
#include <icu/source/common/unicode/uversion.h>
#include <icu/source/common/unicode/utf8.h>
#include <icu/source/common/unicode/unistr.h>
#include <icu/source/common/unicode/schriter.h>
#include <icu/source/common/unicode/rep.h>

#include <xkbcommon/xkbcommon-keysyms.h>

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

	//std::string letter;
	icu::UnicodeString letterUS;

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

	static float squareFunctionHarmonics(float x, int count) {
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

		uint32_t cursor_loop_len = 30;
		cursor_phase = (cursor_phase + 1) % cursor_loop_len;
		float cursor_alpha = 0.5 * sin(2 * M_PI * (float)cursor_phase / cursor_loop_len) + 0.5;
		auto cursor_color = SkColor4f({ 
			color.fR,
			color.fG,
			color.fB,
			cursor_alpha });
		auto cursor_paint = SkPaint(cursor_color);

		canvas.clear(color);

		SkRect rect = SkRect::MakeLTRB(
			w * 0.9 / 5, h / 5, 
			w * 4.1 / 5, 4 * h / 5);
		canvas.drawRect(rect, paint2);

		int sz = (int)(0.8 * h / sqrt(4 * fmax(1, letterUS.countChar32() - 2)));

		std::vector<SkString> fontFamilies { SkString("Noto Sans"), SkString("Noto Sans Hebrew") };

		skia::textlayout::TextStyle style;
        //style.setBackgroundColor(paint2);
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

		auto letterUSWithSpace = letterUS;

		const icu::UnicodeString emptySpace = u"\u200B";
		
		if (letterUS.isEmpty() || 
		    letterUS.endsWith(u"\u000A")) {   // Adding empty space to the end in case of a newline

			letterUSWithSpace = letterUS + emptySpace;
		} else {
			letterUSWithSpace = letterUS;
		}

		std::string letterU8;
		letterU8 = letterUSWithSpace.toUTF8String(letterU8);
		builder.addText(letterU8.c_str(), letterU8.size());
		
		auto paragraph = builder.Build();
		paragraph->layout(3 * w / 5);

		//Cluster& cluster(ClusterIndex clusterIndex);
    	//ClusterIndex clusterIndex(TextIndex textIndex)
		//paragraph->cluster

		auto text_center_x = w / 5;
		auto text_center_y = h / 2 - paragraph->getHeight() / 2;

		std::string cursorLetter = "A";
		auto typeface = skiaOverlay->getTypefaceForCharacter(cursorLetter[0], fontFamilies, style.getFontStyle());
		auto font = std::make_shared<SkFont>(typeface, sz);
		SkFontMetrics curFontMetrics;
		font->getMetrics(&curFontMetrics);


		// SkRect cursorBounds;
		// font->measureText(cursorLetter.c_str(), cursorLetter.size(), SkTextEncoding::kUTF8, &cursorBounds);
		

		if (letterU8.size() > 0) {
			uint32_t text_len = letterUSWithSpace.countChar32();
			std::vector<skia::textlayout::TextBox> boxes = paragraph->getRectsForRange(text_len - 1, text_len,//prev_pos, letter.size(),
													skia::textlayout::RectHeightStyle::kMax,
													skia::textlayout::RectWidthStyle::kTight);
			paragraph->paint(&canvas, text_center_x, text_center_y);

			skia::textlayout::Paragraph::GlyphInfo glyphInfo;
			bool gcres = paragraph->getGlyphInfoAtUTF16Offset(text_len - 1, &glyphInfo);
			
			//std::cout << gcres << "; " << (glyphInfo.fDirection == skia::textlayout::TextDirection::kRtl ? "RTL" : "LTR") << std::endl;
			bool rtl = gcres && (glyphInfo.fDirection == skia::textlayout::TextDirection::kRtl);

			if (boxes.size() > 0) {
				auto& box = *boxes.rbegin();
				auto rect = box.rect;
				rect.offset(text_center_x, text_center_y);

				uint32_t curWidth = 4;				
				if (rtl) {
					rect.fRight = rect.fLeft + curWidth;
				} else {
					rect.fLeft = rect.fRight - curWidth;
				}
				
				canvas.drawRect(rect, cursor_paint);
			}
		}


		// canvas.drawString(letter.c_str(), 
		// 		w / 2 - letterBounds.centerX(), //.width() / 2, 
		// 		h / 2 - (letterMetrics.fAscent + letterMetrics.fDescent) / 2, *font, paint);
    }

    void onCharacter(pi::Keyboard& kbd, char32_t charCode) override { 
		if (/*utf8CharCode[0]*/charCode <= 0x1F || /*utf8CharCode[0]*/charCode == 0x7F) {
			std::stringstream ss;
			uint32_t code = charCode;//reinterpret_cast<uint8_t&>(utf8CharCode[0]);
			ss << "0x" << std::setfill ('0') << std::setw(2) << std::hex << code;
			//letter = ss.str();
		} else {
			letterUS.append((UChar32)charCode);// += utf8CharCode;
		}
	}
    void onKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat) override { 
		if (keysym == XKB_KEY_Escape) {
			platform.lock()->stop();
		} else if (keysym == XKB_KEY_Return) {
			letterUS += u"\u000A"; // Carriage return
		} else if (keysym == XKB_KEY_BackSpace) {
			auto sz = letterUS.countChar32();
			if (sz > 0) {
				int32_t pos = sz - 1;
				letterUS = letterUS.remove(pos);
				/*U8_BACK_1((uint8_t*)letter.c_str(), 0, pos);

				letter = letter.substr(0, pos);*/
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
