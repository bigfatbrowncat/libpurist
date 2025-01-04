#include <purist/graphics/skia/TextInput.h>

#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skparagraph/include/FontCollection.h>
#include <modules/skparagraph/include/TextStyle.h>
#include <modules/skparagraph/include/ParagraphBuilder.h>

namespace purist::graphics::skia {

namespace st = ::skia::textlayout;
namespace pi = purist::input;

void TextInput::drawIntoSurface(std::shared_ptr<Display> display, std::shared_ptr<SkiaOverlay> skiaOverlay, SkCanvas& canvas) {
    std::vector<SkString> fontFamilies { SkString("Noto Sans"), SkString("Noto Sans Hebrew") };

    cursor_phase = (cursor_phase + 1) % cursor_loop_len;
    float cursor_alpha = 0.5 * sin(2 * M_PI * (float)cursor_phase / cursor_loop_len) + 0.5;
    auto color = paint.getColor4f();
    auto cursor_color = SkColor4f({ 
        color.fR,
        color.fG,
        color.fB,
        cursor_alpha });
    auto cursor_paint = SkPaint(cursor_color);


    st::TextStyle style;
    style.setForegroundColor(paint);
    style.setFontFamilies(fontFamilies);
    style.setFontSize(fontSize);
    
    st::ParagraphStyle paraStyle;
    paraStyle.setHeight(fontSize);
    paraStyle.setTextStyle(style);
    paraStyle.setTextAlign(st::TextAlign::kCenter);
    auto fontCollection = sk_make_sp<st::FontCollection>();
    fontCollection->setDefaultFontManager(skiaOverlay->getFontMgr());
    st::ParagraphBuilderImpl builder(paraStyle, fontCollection);

    auto letterUSWithSpace = letterUS;

    const icu::UnicodeString emptySpace = u"\u200B";
    
    // Adding zero-length space to the end 
    // if the string is empty or ends with a newline
    if (letterUS.isEmpty() || 
        letterUS.endsWith(u"\u000A")) {   

        letterUSWithSpace = letterUS + emptySpace;
    } else {
        letterUSWithSpace = letterUS;
    }

    std::string letterU8;
    letterU8 = letterUSWithSpace.toUTF8String(letterU8);
    builder.addText(letterU8.c_str(), letterU8.size());
    
    auto paragraph = builder.Build();
    paragraph->layout(width/*3 * w / 5*/);

    auto text_left_x = position_x; //w / 5;
    auto text_base_y = /*h / 2*/ position_y - paragraph->getHeight() / 2;

    std::string cursorLetter = "A";
    auto typeface = skiaOverlay->getTypefaceForCharacter(cursorLetter[0], fontFamilies, style.getFontStyle());
    auto font = std::make_shared<SkFont>(typeface, fontSize);
    SkFontMetrics curFontMetrics;
    font->getMetrics(&curFontMetrics);
    
    if (letterU8.size() > 0) {
        uint32_t text_len = letterUSWithSpace.countChar32();
        std::vector<st::TextBox> boxes = paragraph->getRectsForRange(text_len - 1, text_len,//prev_pos, letter.size(),
                                                st::RectHeightStyle::kMax,
                                                st::RectWidthStyle::kTight);
        paragraph->paint(&canvas, text_left_x, text_base_y);

        st::Paragraph::GlyphInfo glyphInfo;
        bool gcres = paragraph->getGlyphInfoAtUTF16Offset(text_len - 1, &glyphInfo);
        
        //std::cout << gcres << "; " << (glyphInfo.fDirection == st::TextDirection::kRtl ? "RTL" : "LTR") << std::endl;
        bool rtl = gcres && (glyphInfo.fDirection == st::TextDirection::kRtl);

        if (boxes.size() > 0) {
            auto& box = *boxes.rbegin();
            auto rect = box.rect;
            rect.offset(text_left_x, text_base_y);

            uint32_t curWidth = 4;				
            if (rtl) {
                rect.fRight = rect.fLeft + curWidth;
            } else {
                rect.fLeft = rect.fRight - curWidth;
            }
            
            canvas.drawRect(rect, cursor_paint);
        }
    }
}

bool TextInput::onCharacter(pi::Keyboard& kbd, char32_t charCode) { 
    if (/*utf8CharCode[0]*/charCode <= 0x1F || /*utf8CharCode[0]*/charCode == 0x7F) {
        // std::stringstream ss;
        // uint32_t code = charCode;//reinterpret_cast<uint8_t&>(utf8CharCode[0]);
        // ss << "0x" << std::setfill ('0') << std::setw(2) << std::hex << code;
    } else {
        letterUS.append((UChar32)charCode);
        return true;
    }
    return false;
}

bool TextInput::onKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat) { 
    if (keysym == XKB_KEY_Return) {
        letterUS += u"\u000A"; // Carriage return
        return true;

    } else if (keysym == XKB_KEY_BackSpace) {
        auto sz = letterUS.countChar32();
        if (sz > 0) {
            int32_t pos = sz - 1;
            letterUS = letterUS.remove(pos);
            /*U8_BACK_1((uint8_t*)letter.c_str(), 0, pos);

            letter = letter.substr(0, pos);*/
        }
        return true;
    }

    return false;
}


}