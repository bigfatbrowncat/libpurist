#pragma once

#include "TermSubprocess.h"
#include "TextCellsMatrixModel.h"
#include "cells.h"

// VTerm headers
#include <vterm.h>
#include <vterm_keycodes.h>

// libpurist headers
#include <purist/input/KeyboardHandler.h>

// Skia headers
#include <include/core/SkColor.h>
#include <include/core/SkPicture.h>

// C++ std headers
#include <vector>
#include <string>
#include <memory>
#include <optional>

namespace pi = purist::input;

class SkiaTermEmulator;

class VTermWrapper : public TextCellsMatrixModel {
private:
    std::shared_ptr<TermSubprocess> subprocess;
    std::weak_ptr<SkiaTermEmulator> frontend;

    VTerm* vterm;
    VTermScreen* screen;

    uint32_t rows, cols;

    VTermPos cursor_pos;

    VTermColor color_palette[16];

    std::string apc_buffer;
    sk_sp<SkPicture> picture;

    cells<std::optional<TextCell>> text_cells;

    int pushed_lines = 0;

    int damage(int start_row, int start_col, int end_row, int end_col);
    int moverect(VTermRect dest, VTermRect src);
    int movecursor(VTermPos pos, VTermPos oldpos, int visible);
    int settermprop(VTermProp prop, VTermValue *val);
    int bell();
    int resize(int rows, int cols);
    int sb_pushline(int cols, const VTermScreenCell *cells);
    int sb_popline(int cols, VTermScreenCell *cells);

    int device_control_string(const char *command, size_t commandlen, VTermStringFragment frag);
    int application_program_command(VTermStringFragment frag);

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

    static int vterm_fb_control(unsigned char control, void *user);
    static int vterm_fb_csi(const char *leader, const long args[], int argcount, const char *intermed, char command, void *user);
    static int vterm_fb_osc(int command, VTermStringFragment frag, void *user);
    static int vterm_fb_dcs(const char *command, size_t commandlen, VTermStringFragment frag, void *user);
    static int vterm_fb_apc(VTermStringFragment frag, void *user);
    static int vterm_fb_pm(VTermStringFragment frag, void *user);
    static int vterm_fb_sos(VTermStringFragment frag, void *user);

    const VTermStateFallbacks state_fallbacks = {
        vterm_fb_control,
        vterm_fb_csi,
        vterm_fb_osc,
        vterm_fb_dcs,
        vterm_fb_apc,
        vterm_fb_pm,
        vterm_fb_sos
    };

    void setStandardColorPalette(VTermState* state);
    void keyboard_unichar(uint32_t c, VTermModifier mod);
    void keyboard_key(VTermKey key, VTermModifier mod);
    void input_write(const char* bytes, size_t len);

public:
    void processCharacter(pi::Keyboard& kbd, char32_t charCode, pi::Modifiers mods, pi::Leds leds);
    void processKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat);

    TextCell getCell(int32_t row, int32_t col) override;
    static void output_callback(const char* s, size_t len, void* user);

    void processInputFromSubprocess();

    TextCellsPos getCursorPos() override { 
        return TextCellsPos { cursor_pos.row, cursor_pos.col }; 
    }

    sk_sp<SkPicture> getPicture() override {
        return picture;
    }

    //int getPushedLines() const { return pushed_lines; }

    VTermWrapper(uint32_t _rows, uint32_t _cols, 
        std::shared_ptr<TermSubprocess> subprocess,
        std::weak_ptr<SkiaTermEmulator> frontend);
    virtual ~VTermWrapper();
};
