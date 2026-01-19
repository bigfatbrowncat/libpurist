#pragma once

#include "TermSubprocess.h"

// VTerm headers
#include <vterm.h>
#include <vterm_keycodes.h>

// libpurist headers
#include <purist/input/KeyboardHandler.h>

// Skia headers
#include <include/core/SkColor.h>

// C++ std headers
#include <vector>

namespace pi = purist::input;

struct VTermScreenCellWrapper {
  std::vector<uint32_t> chars;
  char width;
  VTermScreenCellAttrs attrs;
  SkColor4f foreColor, backColor;
};

class SkiaTermEmulator;

class VTermWrapper {
    std::shared_ptr<TermSubprocess> subprocess;
    std::weak_ptr<SkiaTermEmulator> frontend;

    VTerm* vterm;
    VTermScreen* screen;

    uint32_t rows, cols;

    VTermPos cursor_pos;

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

public:
    void processCharacter(pi::Keyboard& kbd, char32_t charCode, pi::Modifiers mods, pi::Leds leds);
    void processKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat);

    VTermScreenCellWrapper getCell(int32_t row, int32_t col);
    static void output_callback(const char* s, size_t len, void* user);

    void processInputFromSubprocess();

    VTermPos getCursorPos() { return cursor_pos; }

    VTermWrapper(uint32_t _rows, uint32_t _cols, 
        std::shared_ptr<TermSubprocess> subprocess,
        std::weak_ptr<SkiaTermEmulator> frontend);
    virtual ~VTermWrapper();
};
