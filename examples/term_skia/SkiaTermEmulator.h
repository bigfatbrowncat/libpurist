#pragma once

#include "TermSubprocess.h"

#include "lru_cache.h"
#include "cells.h"
#include "row_key.h"

// libpurist headers
#include <purist/graphics/skia/DisplayContentsSkia.h>
#include <purist/graphics/Display.h>
#include <purist/graphics/Mode.h>
#include <purist/input/KeyboardHandler.h>
#include <purist/Platform.h>
#include <purist/graphics/skia/icu_common.h>

// Skia headers
#include "include/core/SkScalar.h"
#include "include/core/SkRect.h"
#include <include/core/SkCanvas.h>
#include <include/core/SkSurface.h>
#include <include/core/SkBitmap.h>
#include <include/core/SkImage.h>
#include <include/core/SkPaint.h>
#include <include/core/SkFont.h>
#include <include/core/SkData.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkColor.h>

// xkbcommon headers
#include <xkbcommon/xkbcommon-keysyms.h>

// VTerm headers
#include <vterm.h>
#include <vterm_keycodes.h>

// C++ std headers
#include <map>
#include <memory>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <chrono>
#include <optional>
#include <mutex>
#include <thread>
#include <atomic>

namespace p = purist;
namespace pg = purist::graphics;
namespace pi = purist::input;
namespace pgs = purist::graphics::skia;


class SkiaTermEmulator {
private:
    struct SurfaceAndImage {
        sk_sp<SkSurface> surface;
        sk_sp<SkImage> image;
    };

    std::shared_ptr<TermSubprocess> subprocess;

    float cursorPhase = 0;
    uint32_t cursorBlinkPeriodMSec = 300;

    sk_sp<SkTypeface> typeface;
    std::shared_ptr<SkFont> font;
    SkScalar font_width;
    SkScalar font_height;
    SkScalar font_descent;
    uint32_t ringingFramebuffers = 0;
    bool cursorVisible = true;
    bool cursorBlink = true;

    VTerm* vterm;
    VTermScreen* screen;

    uint32_t rows, cols;
    int divider = 4;

    lru_cache<row_key, SurfaceAndImage> typesettingBox;
    std::vector<sk_sp<SkSurface>> letter_surfaces;
    std::map<uint32_t, std::shared_ptr<cells<unsigned char>>> screenUpdateMatrices;  // The key is the display connector id

    VTermPos cursor_pos;
    uint32_t framebuffersCount = 0;

    VTermColor color_palette[16];

    int damage(int start_row, int start_col, int end_row, int end_col);
    int moverect(VTermRect dest, VTermRect src);
    int movecursor(VTermPos pos, VTermPos oldpos, int visible);
    int settermprop(VTermProp prop, VTermValue *val);
    int bell();
    int resize(int rows, int cols);
    int sb_pushline(int cols, const VTermScreenCell *cells);
    int sb_popline(int cols, VTermScreenCell *cells);

    static int vterm_cb_damage(VTermRect rect, void *user);
    static int vterm_cb_moverect(VTermRect dest, VTermRect src, void *user);
    static int vterm_cb_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user);
    static int vterm_cb_settermprop(VTermProp prop, VTermValue *val, void *user);
    static int vterm_cb_bell(void *user);
    static int vterm_cb_resize(int rows, int cols, void *user);
    static int vterm_cb_sb_pushline(int cols, const VTermScreenCell *cells, void *user);
    static int vterm_cb_sb_popline(int cols, VTermScreenCell *cells, void *user);

    const VTermScreenCallbacks screen_callbacks = {
        vterm_cb_damage,
        vterm_cb_moverect,
        vterm_cb_movecursor,
        vterm_cb_settermprop,
        vterm_cb_bell,
        vterm_cb_resize,
        vterm_cb_sb_pushline,
        vterm_cb_sb_popline
    };

    void setStandardColorPalette(VTermState* state);
    void keyboard_unichar(uint32_t c, VTermModifier mod);
    void keyboard_key(VTermKey key, VTermModifier mod);
    void input_write(const char* bytes, size_t len);

    static void output_callback(const char* s, size_t len, void* user);
    sk_sp<SkImage> drawCells(int col_min, int col_max, int row, //int row_min, int row_max, 
                   int buffer_width, int buffer_height,
                   std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, const icu::Normalizer2* normalizer);

public:
    SkiaTermEmulator(uint32_t _rows, uint32_t _cols, std::shared_ptr<TermSubprocess> subprocess);

    void setTypeface(sk_sp<SkTypeface> typeface);
    void drawIntoSurface(std::shared_ptr<pg::Display> display, 
                         std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, 
                         int width, int height, SkCanvas& canvas, bool refreshed);
    void processCharacter(pi::Keyboard& kbd, char32_t charCode, pi::Modifiers mods, pi::Leds leds);
    void processKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat);

    ~SkiaTermEmulator();
};
