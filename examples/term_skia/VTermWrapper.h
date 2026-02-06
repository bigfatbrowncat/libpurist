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
#include <mutex>

namespace pi = purist::input;

class SkiaTermRenderer;

class VTermWrapperUpdate : public TextCellsDataUpdate {
protected:
    friend class VTermWrapper;

    std::optional<TextCellsPos> cursor_pos = std::nullopt;
    std::optional<sk_sp<SkPicture>> picture = std::nullopt;
    cells<std::optional<TextCell>> text_cells;
    std::optional<bool> cursorVisible = std::nullopt;
    std::optional<bool> cursorBlink = std::nullopt;
    int pictureShiftedUpLines = 0;
    bool bellIsSet = false;
    
public:
    VTermWrapperUpdate(int _rows, int _cols) : text_cells(_rows, _cols) { }

    std::optional<TextCell> getCell(int32_t row, int32_t col) override;
    std::optional<TextCellsPos> getCursorPos() override { 
        return cursor_pos; //TextCellsPos { cursor_pos.row, cursor_pos.col }; 
    }

    std::optional<sk_sp<SkPicture>> getPicture() override {
        return picture;
    }

    int getPictureShiftedUpLines() override {
        return pictureShiftedUpLines;
    }

    bool isBellSet() override { return bellIsSet; }

    std::optional<bool> isCursorVisible() override {
        return cursorVisible;
    }

    std::optional<bool> isCursorBlink() override {
        return cursorBlink;
    }

};


class VTermWrapper : public TextCellsMatrixModel {
private:
    std::shared_ptr<TermSubprocess> subprocess;
    //std::weak_ptr<SkiaTermEmulator> frontend;

    std::mutex swapMutex;

    VTerm* vterm;
    VTermScreen* screen;

    uint32_t rows, cols;

    VTermColor color_palette[16];

    std::string apc_buffer;

    std::shared_ptr<VTermWrapperUpdate> updateInProgress;

    int pushed_lines = 0;

    // Modifying functions
    int damage(int start_row, int start_col, int end_row, int end_col);
    int moverect(VTermRect dest, VTermRect src);
    int movecursor(VTermPos pos, VTermPos oldpos, int visible);
    int settermprop(VTermProp prop, VTermValue *val);
    int bell();
    int resize(int rows, int cols);
    int sb_pushline(int cols, const VTermScreenCell *cells);
    int sb_popline(int cols, VTermScreenCell *cells);
    int sb_clear();
    int device_control_string(const char *command, size_t commandlen, VTermStringFragment frag);
    int application_program_command(VTermStringFragment frag);

    // Callback handlers
    static int vterm_cb_damage(VTermRect rect, void *user);
    static int vterm_cb_moverect(VTermRect dest, VTermRect src, void *user);
    static int vterm_cb_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user);
    static int vterm_cb_settermprop(VTermProp prop, VTermValue *val, void *user);
    static int vterm_cb_bell(void *user);
    static int vterm_cb_resize(int rows, int cols, void *user);
    static int vterm_cb_sb_pushline(int cols, const VTermScreenCell *cells, void *user);
    static int vterm_cb_sb_popline(int cols, VTermScreenCell *cells, void *user);
    static int vterm_cb_sb_clear(void *user);

    const VTermScreenCallbacks screen_callbacks = {
        vterm_cb_damage,
        vterm_cb_moverect,
        vterm_cb_movecursor,
        vterm_cb_settermprop,
        vterm_cb_bell,
        vterm_cb_resize,
        vterm_cb_sb_pushline,
        vterm_cb_sb_popline,
        vterm_cb_sb_clear
    };

    // State Fallback handlers
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

    //////////////

    void setStandardColorPalette(VTermState* state);
    void keyboard_unichar(uint32_t c, VTermModifier mod);
    void keyboard_key(VTermKey key, VTermModifier mod);
    void input_write(const char* bytes, size_t len);

public:
    void processCharacter(pi::Keyboard& kbd, char32_t charCode, pi::Modifiers mods, pi::Leds leds);
    void processKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat);

    std::shared_ptr<TextCellsDataUpdate> getContentsUpdate() override {
        std::lock_guard<std::mutex> lock { swapMutex };
        std::shared_ptr<TextCellsDataUpdate> dataReady = updateInProgress;
        updateInProgress = std::make_shared<VTermWrapperUpdate>(rows, cols);
        return dataReady;
    }

    static void output_callback(const char* s, size_t len, void* user);

    bool processInputFromSubprocess();
    void refreshCell(int32_t row, int32_t col);
    void refreshDataRect(int start_row, int start_col, int end_row, int end_col);

    //int getPushedLines() const { return pushed_lines; }

    VTermWrapper(uint32_t _rows, uint32_t _cols, 
        std::shared_ptr<TermSubprocess> subprocess/*,
        std::weak_ptr<SkiaTermEmulator> frontend*/);
        
    virtual ~VTermWrapper();
};
