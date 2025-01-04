#pragma once

#include <purist/graphics/Display.h>
#include <purist/graphics/skia/SkiaOverlay.h>
#include <purist/graphics/skia/icu_common.h>
#include <purist/input/KeyboardHandler.h>

#include <include/core/SkPaint.h>
#include <include/core/SkCanvas.h>

#include <xkbcommon/xkbcommon-keysyms.h>

#include <memory>

namespace purist::graphics::skia {

class TextInput {
private:
	icu::UnicodeString text;
    SkPaint paint;
    uint32_t fontSize;
    uint32_t width, height;

    uint32_t cursor_loop_len = 30, cursor_phase;
    uint32_t left, top;

public:
    void setWidth(uint32_t width) { this->width = width; }
    void setHeight(uint32_t height) { this->height = height; }
    void setLeft(uint32_t x) { left = x; }
    void setTop(uint32_t y) { top = y; }
    void setPaint(const SkPaint& paint) { this->paint = paint; }
    void setFontSize(uint32_t sz) { fontSize = sz; }

    const icu::UnicodeString& getText() const { return text; }

    void drawIntoSurface(std::shared_ptr<Display> display, std::shared_ptr<SkiaOverlay> skiaOverlay, SkCanvas& canvas);

    bool onCharacter(purist::input::Keyboard& kbd, char32_t charCode);
    bool onKeyPress(purist::input::Keyboard& kbd, uint32_t keysym, 
                    purist::input::Modifiers mods, purist::input::Leds leds, bool repeat);

};

}