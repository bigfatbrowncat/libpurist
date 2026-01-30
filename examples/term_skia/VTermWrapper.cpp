#include "VTermWrapper.h"

#include "TextCellsMatrixModel.h"
#include "include/core/SkPicture.h"
#include "include/core/SkData.h"
#include "base64.hpp"

// libpurist headers
#include <purist/graphics/skia/icu_common.h>

// xkbcommon headers
#include <xkbcommon/xkbcommon-keysyms.h>

#include <iostream>
#include <memory>
#include <string>

void VTermWrapper::refreshDataRect(int start_row, int start_col, int end_row, int end_col) {
    for (int row = start_row; row < end_row; row++) {
        for (int col = start_col; col < end_col; col++) {
            refreshCell(row, col);
        }
    }
}

int VTermWrapper::damage(int start_row, int start_col, int end_row, int end_col) {
    std::lock_guard<std::mutex> lock { swapMutex };
    this->refreshDataRect(start_row, start_col, end_row, end_col);

    //frontend.lock()->refreshRect(start_row, start_col, end_row, end_col);
    return 0;
}

int VTermWrapper::moverect(VTermRect dest, VTermRect src) {
    std::lock_guard<std::mutex> lock { swapMutex };
    return 0;
}

int VTermWrapper::movecursor(VTermPos pos, VTermPos oldpos, int visible) {
    std::lock_guard<std::mutex> lock { swapMutex };
    // Issuing repainting for the old cursor position
    // frontend.lock()->refreshRect(
    //     dataInProgress->cursor_pos.row, 
    //     dataInProgress->cursor_pos.col, 
    //     dataInProgress->cursor_pos.row + 1, 
    //     dataInProgress->cursor_pos.col + 1);

    updateInProgress->cursor_pos = TextCellsPos { pos.row, pos.col };
    
    // Issuing repainting for the new cursor position
    // frontend.lock()->refreshRect(
    //     dataInProgress->cursor_pos.row,
    //     dataInProgress->cursor_pos.col, 
    //     dataInProgress->cursor_pos.row + 1, 
    //     dataInProgress->cursor_pos.col + 1);
        
    return 0;
}
int VTermWrapper::settermprop(VTermProp prop, VTermValue *val) {
    std::lock_guard<std::mutex> lock { swapMutex };
    switch (prop) {
    case VTERM_PROP_CURSORVISIBLE:
        updateInProgress->cursorVisible = val->boolean;
        //frontend.lock()->setCursorVisible(val->boolean);
        return true;
        break;
    case VTERM_PROP_CURSORBLINK:
        updateInProgress->cursorBlink = val->boolean;
        //frontend.lock()->setCursorBlink(val->boolean);
        return true;
        break;

    case VTERM_PROP_CURSORSHAPE:
        break;

    default:
        break;
    }
    return 0;
}

int VTermWrapper::bell() {
    std::lock_guard<std::mutex> lock { swapMutex };
    //frontend.lock()->bellBlink();
    return 0;
}
int VTermWrapper::resize(int rows, int cols) {
    std::lock_guard<std::mutex> lock { swapMutex };
    return 0;
}

int VTermWrapper::sb_pushline(int cols, const VTermScreenCell *cells) {
    std::lock_guard<std::mutex> lock { swapMutex };
    pushed_lines ++;
    return 0;
}

int VTermWrapper::sb_popline(int cols, VTermScreenCell *cells) {
    std::lock_guard<std::mutex> lock { swapMutex };
    return 0;
}

int VTermWrapper::device_control_string(const char *command, size_t commandlen, VTermStringFragment frag) {
    std::lock_guard<std::mutex> lock { swapMutex };
    std::string dcs(command, commandlen);
    std::cout << "DCS: " << dcs << std::endl;
    return 1;
}

int VTermWrapper::application_program_command(VTermStringFragment frag) {
    std::lock_guard<std::mutex> lock { swapMutex };
    std::string apc(frag.str, frag.len);

    const std::string OPEN_MARK { "<skpicture>" };
    const std::string CLOSE_MARK { "</skpicture>" };

    bool complete = false;
    std::string::size_type pos = 0;
    ssize_t open_pos = -1;
    ssize_t close_pos = -1;
    do {
        if (apc.substr(pos, OPEN_MARK.size()) == OPEN_MARK) {
            //pos += OPEN_MARK.size();
            open_pos = pos;
        }
        if (apc.substr(pos, CLOSE_MARK.size()) == CLOSE_MARK) {
            pos += CLOSE_MARK.size();
            close_pos = pos;
        }
        pos = apc.find("<", pos + 1);
    } while (pos != std::string::npos);

    if (open_pos == -1 && close_pos == -1) {
        // We neither caught opening nor closing. Continuing the buffer
        apc_buffer += apc;
        complete = false;
    } else if (open_pos == -1 && close_pos >= 0) {
        // We caught only closing. Finalizing the buffer with the start of the message
        apc_buffer += apc.substr(0, close_pos);
        complete = true;
    } else if (open_pos >= 0 && close_pos == -1) {
        // We caught only opening. Resetting the buffer to the tail of the message
        apc_buffer = apc.substr(open_pos);
        complete = false;
    } else if (open_pos >= 0 && close_pos > open_pos) {
        // The full buffer is inside
        apc_buffer += apc.substr(open_pos, close_pos - open_pos);
        complete = true;
    }

    if (complete) {
        if (apc_buffer.substr(0, OPEN_MARK.size()) == OPEN_MARK && 
            apc_buffer.substr(apc_buffer.size() - CLOSE_MARK.size()) == CLOSE_MARK) {

            apc_buffer = apc_buffer.substr(OPEN_MARK.size(), apc_buffer.size() - OPEN_MARK.size() - CLOSE_MARK.size());
            
            auto decoded = base64::from_base64(apc_buffer);
            
            sk_sp<SkData> data = SkData::MakeWithCopy(
                decoded.data(), 
                decoded.size()
            );

            // 2. Deserialize the data into an SkPicture
            updateInProgress->picture = SkPicture::MakeFromData(
                data->data(), 
                data->size()
            );
        }

    }

    return 1;
}

/////////////////////

int VTermWrapper::vterm_cb_damage(VTermRect rect, void *user)
{
    return ((VTermWrapper*)user)->damage(rect.start_row, rect.start_col, rect.end_row, rect.end_col);
}

int VTermWrapper::vterm_cb_moverect(VTermRect dest, VTermRect src, void *user)
{
    return ((VTermWrapper*)user)->moverect(dest, src);
}

int VTermWrapper::vterm_cb_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user)
{
    return ((VTermWrapper*)user)->movecursor(pos, oldpos, visible);
}

int VTermWrapper::vterm_cb_settermprop(VTermProp prop, VTermValue *val, void *user)
{
    return ((VTermWrapper*)user)->settermprop(prop, val);
}

int VTermWrapper::vterm_cb_bell(void *user)
{
    return ((VTermWrapper*)user)->bell();
}

int VTermWrapper::vterm_cb_resize(int rows, int cols, void *user)
{
    return ((VTermWrapper*)user)->resize(rows, cols);
}

int VTermWrapper::vterm_cb_sb_pushline(int cols, const VTermScreenCell *cells, void *user)
{
    return ((VTermWrapper*)user)->sb_pushline(cols, cells);
}

int VTermWrapper::vterm_cb_sb_popline(int cols, VTermScreenCell *cells, void *user)
{
    return ((VTermWrapper*)user)->sb_popline(cols, cells);
}


int VTermWrapper::vterm_fb_control(unsigned char control, void *user) {
    return 0;
}

int VTermWrapper::vterm_fb_csi(const char *leader, const long args[], int argcount, const char *intermed, char command, void *user) {
    return 0;
}

int VTermWrapper::vterm_fb_osc(int command, VTermStringFragment frag, void *user) {
    return 0;
}

int VTermWrapper::vterm_fb_dcs(const char *command, size_t commandlen, VTermStringFragment frag, void *user) {
    return ((VTermWrapper*)user)->device_control_string(command, commandlen, frag);
}

int VTermWrapper::vterm_fb_apc(VTermStringFragment frag, void *user) {
    return ((VTermWrapper*)user)->application_program_command(frag);
}

int VTermWrapper::vterm_fb_pm(VTermStringFragment frag, void *user) {
    return 0;
}

int VTermWrapper::vterm_fb_sos(VTermStringFragment frag, void *user) {
    return 0;
}


void VTermWrapper::setStandardColorPalette(VTermState* state) {
    VTermColor black;
    vterm_color_rgb(&black, 1, 1, 1);
    VTermColor red;
    vterm_color_rgb(&red, 222, 56, 43);
    VTermColor green;
    vterm_color_rgb(&green, 57, 181, 74);
    VTermColor yellow;
    vterm_color_rgb(&yellow, 255, 199, 6);
    VTermColor blue;
    vterm_color_rgb(&blue, 0, 85, 190); //vterm_color_rgb(&blue, 0, 111, 184);
    VTermColor magenta;
    vterm_color_rgb(&magenta, 118, 38, 113);
    VTermColor cyan;
    vterm_color_rgb(&cyan, 30, 160, 200); //vterm_color_rgb(&cyan, 44, 181, 233);
    VTermColor light_gray;
    vterm_color_rgb(&light_gray, 190, 190, 190); //vterm_color_rgb(&light_gray, 204, 204, 204);

    VTermColor dark_gray;
    vterm_color_rgb(&dark_gray, 128, 128, 128);
    VTermColor lred;
    vterm_color_rgb(&lred, 255, 0, 0);
    VTermColor lgreen;
    vterm_color_rgb(&lgreen, 0, 255, 0);
    VTermColor lyellow;
    vterm_color_rgb(&lyellow, 255, 255, 0);
    VTermColor lblue;
    vterm_color_rgb(&lblue, 0, 0, 255);
    VTermColor lmagenta;
    vterm_color_rgb(&lmagenta, 255, 0, 255);
    VTermColor lcyan;
    vterm_color_rgb(&lcyan, 0, 255, 255);
    VTermColor white;
    vterm_color_rgb(&white, 255, 255, 255);

    VTermColor equalized_palette[] = {
        black,     red,  green,  yellow,  blue,  magenta,  cyan,  light_gray,
        dark_gray, lred, lgreen, lyellow, lblue, lmagenta, lcyan, white
    };

    for (uint8_t index = 0; index < 16; index++) {
        color_palette[index] = equalized_palette[index];
        vterm_state_set_palette_color(state, index, &color_palette[index]);
    }
}

void VTermWrapper::keyboard_unichar(uint32_t c, VTermModifier mod) {
    vterm_keyboard_unichar(vterm, c, mod);
}

void VTermWrapper::keyboard_key(VTermKey key, VTermModifier mod) {
    vterm_keyboard_key(vterm, key, mod);
}

void VTermWrapper::input_write(const char* bytes, size_t len) {
    vterm_input_write(vterm, bytes, len);
}

void VTermWrapper::refreshCell(int32_t row, int32_t col) {
    VTermPos pos = { row, col };
    VTermScreenCell cell;
    vterm_screen_get_cell(screen, pos, &cell);

    TextCell res;
    //res.width = cell.width;
    //res.attrs = cell.attrs;

    if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
        vterm_screen_convert_color_to_rgb(screen, &cell.fg);
    }
    if (VTERM_COLOR_IS_RGB(&cell.fg)) {
        res.foreColor = SkColor4f::FromColor(SkColorSetRGB(cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue));
    }

    if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
        vterm_screen_convert_color_to_rgb(screen, &cell.bg);
    }
    if (VTERM_COLOR_IS_RGB(&cell.bg)) {
        res.backColor = SkColor4f::FromColor(SkColorSetRGB(cell.bg.rgb.red, cell.bg.rgb.green, cell.bg.rgb.blue));
    }

    if (cell.attrs.reverse) std::swap(res.foreColor, res.backColor);

    // for (int i = 0; cell.chars[i] != 0 && i < VTERM_MAX_CHARS_PER_CELL; i++) {
    //     res.chars.push_back(cell.chars[i]);
    // }

    if (cell.chars[0] != 0xffffffff) {
        icu::UnicodeString ustr = "";
        for (int i = 0; cell.chars[i] != 0 && i < VTERM_MAX_CHARS_PER_CELL; i++) {
            ustr.append((UChar32)cell.chars[i]);
        }

        //color = cell.foreColor;
        //bgcolor = cell.backColor;

        // if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
        //     vterm_screen_convert_color_to_rgb(screen, &cell.fg);
        // }
        // if (VTERM_COLOR_IS_RGB(&cell.fg)) {
        //     color = SkColor4f::FromColor(SkColorSetRGB(cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue));
        // }
        // // if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
        // //     vterm_screen_convert_color_to_rgb(screen, &cell.bg);
        // // }
        // if (VTERM_COLOR_IS_RGB(&cell.bg)) {
        //     bgcolor = SkColor4f::FromColor(SkColorSetRGB(cell.bg.rgb.red, cell.bg.rgb.green, cell.bg.rgb.blue));
        // }

        
        /* TODO
        int style = TTF_STYLE_NORMAL;
        if (cell.attrs.bold) style |= TTF_STYLE_BOLD;
        if (cell.attrs.underline) style |= TTF_STYLE_UNDERLINE;
        if (cell.attrs.italic) style |= TTF_STYLE_ITALIC;
        if (cell.attrs.strike) style |= TTF_STYLE_STRIKETHROUGH; */
        //if (cell.attrs.blink) { /*TBD*/ }
        
        // SkPaint bgpt(bgcolor);
        // bgpt.setStyle(SkPaint::kFill_Style);
        // canvas.drawRect(rect, bgpt);

        UErrorCode status = U_ZERO_ERROR;
        auto normalizer = icu::Normalizer2::getNFKCInstance(status);
        if (U_FAILURE(status)) throw std::runtime_error("unable to get NFKC normalizer");

        if (ustr.length() > 0) {
            auto ustr_normalized = normalizer->normalize(ustr, status);
            if (U_SUCCESS(status)) {
                ustr_normalized.toUTF8String(res.utf8);
            } else {
                ustr.toUTF8String(res.utf8);
            }
        }
    }
    updateInProgress->text_cells(row, col) = res;
}

std::optional<TextCell> VTermWrapperUpdate::getCell(int32_t row, int32_t col) {
    return text_cells(row, col);
}

void VTermWrapper::output_callback(const char* s, size_t len, void* user)
{
    TermSubprocess* subp = reinterpret_cast<TermSubprocess*>(user);
    std::string str(s, len);
    subp->write(str);
}

bool VTermWrapper::processInputFromSubprocess() {
    return subprocess->readInputAndProcess([&](const std::string_view& input_str) {
        input_write(input_str.data(), input_str.size());
        if (pushed_lines > 2*rows) {
            pushed_lines = 0;
            return false;
        } else {
            return true;
        }
    });
}

VTermWrapper::VTermWrapper(uint32_t _rows, uint32_t _cols, 
                           std::shared_ptr<TermSubprocess> subprocess/*, std::weak_ptr<SkiaTermEmulator> frontend*/)
        : subprocess(subprocess), /*frontend(frontend), */rows(_rows), cols(_cols), 
          updateInProgress(std::make_shared<VTermWrapperUpdate>(_rows, _cols)) {
    
    vterm = vterm_new(_rows, _cols);
    vterm_set_utf8(vterm, 1);
    vterm_output_set_callback(vterm, output_callback, (void*)subprocess.get());

    screen = vterm_obtain_screen(vterm);
    vterm_screen_set_callbacks(screen, &screen_callbacks, this);

    vterm_screen_enable_altscreen(screen, 1);

    VTermState * state = vterm_obtain_state(vterm);

    vterm_state_set_unrecognised_fallbacks(state, &state_fallbacks, this);
    
    setStandardColorPalette(state);

    vterm_screen_set_default_colors(screen, &color_palette[7], &color_palette[0]);
    

    VTermValue val;
    val = { .boolean = true };
    vterm_state_set_termprop(state, VTERM_PROP_CURSORVISIBLE, &val);
    val = { .boolean = true };
    vterm_state_set_termprop(state, VTERM_PROP_CURSORBLINK, &val);
    val = { .number = VTERM_PROP_CURSORSHAPE_BLOCK };
    vterm_state_set_termprop(state, VTERM_PROP_CURSORSHAPE, &val);

    vterm_screen_reset(screen, 1);
}

void VTermWrapper::processCharacter(pi::Keyboard& kbd, char32_t charCode, pi::Modifiers mods, pi::Leds leds) { 
            
    int mod = VTERM_MOD_NONE;
    if (mods.ctrl) mod |= VTERM_MOD_CTRL;
    if (mods.alt) mod |= VTERM_MOD_ALT;
    if (mods.shift) mod |= VTERM_MOD_SHIFT;


    //for (int i = 0; i < strlen(ev.text.text); i++) {
    keyboard_unichar(charCode, (VTermModifier)mod);   //ev.text.text[i], (VTermModifier)mod);
    //}

}

void VTermWrapper::processKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat) { 

    int mod = VTERM_MOD_NONE;
    if (mods.ctrl) mod |= VTERM_MOD_CTRL;
    if (mods.alt) mod |= VTERM_MOD_ALT;
    if (mods.shift) mod |= VTERM_MOD_SHIFT;


    switch (keysym) {
    case XKB_KEY_Return:
    case XKB_KEY_KP_Enter:
        keyboard_key(VTERM_KEY_ENTER, (VTermModifier)mod);
        break;
    case XKB_KEY_Tab:
        keyboard_key(VTERM_KEY_TAB, (VTermModifier)mod);
        break;
    case XKB_KEY_BackSpace:
        keyboard_key(VTERM_KEY_BACKSPACE, (VTermModifier)mod);
        break;
    case XKB_KEY_Escape:
        keyboard_key(VTERM_KEY_ESCAPE, (VTermModifier)mod);
        break;

    case XKB_KEY_Up:
    case XKB_KEY_KP_Up:
        keyboard_key(VTERM_KEY_UP, (VTermModifier)mod);
        break;
    case XKB_KEY_Down:
    case XKB_KEY_KP_Down:
        keyboard_key(VTERM_KEY_DOWN, (VTermModifier)mod);
        break;
    case XKB_KEY_Left:
    case XKB_KEY_KP_Left:
        keyboard_key(VTERM_KEY_LEFT, (VTermModifier)mod);
        break;
    case XKB_KEY_Right:
    case XKB_KEY_KP_Right:
        keyboard_key(VTERM_KEY_RIGHT, (VTermModifier)mod);
        break;

    case XKB_KEY_Insert:
    case XKB_KEY_KP_Insert:
        keyboard_key(VTERM_KEY_INS, (VTermModifier)mod);        
        break;
    case XKB_KEY_Delete:
    case XKB_KEY_KP_Delete:
        keyboard_key(VTERM_KEY_DEL, (VTermModifier)mod);
        break;
    case XKB_KEY_Home:
    case XKB_KEY_KP_Home:
        keyboard_key(VTERM_KEY_HOME, (VTermModifier)mod);
        break;
    case XKB_KEY_End:
    case XKB_KEY_KP_End:
        keyboard_key(VTERM_KEY_END, (VTermModifier)mod);
        break;
    case XKB_KEY_Page_Up:
    case XKB_KEY_KP_Page_Up:
        keyboard_key(VTERM_KEY_PAGEUP, (VTermModifier)mod);
        break;
    case XKB_KEY_Page_Down:
    case XKB_KEY_KP_Page_Down:
        keyboard_key(VTERM_KEY_PAGEDOWN, (VTermModifier)mod);
        break;
    
    case XKB_KEY_KP_Multiply:
        keyboard_key(VTERM_KEY_KP_MULT, (VTermModifier)mod);
        break;
    case XKB_KEY_KP_Add:
        keyboard_key(VTERM_KEY_KP_PLUS, (VTermModifier)mod);
        break;
    case XKB_KEY_KP_Separator:
        keyboard_key(VTERM_KEY_KP_COMMA, (VTermModifier)mod);
        break;
    case XKB_KEY_KP_Subtract:
        keyboard_key(VTERM_KEY_KP_MINUS, (VTermModifier)mod);
        break;
    case XKB_KEY_KP_Decimal:
        keyboard_key(VTERM_KEY_KP_PERIOD, (VTermModifier)mod);
        break;
    case XKB_KEY_KP_Divide:
        keyboard_key(VTERM_KEY_KP_DIVIDE, (VTermModifier)mod);
        break;
    case XKB_KEY_KP_Equal:
        keyboard_key(VTERM_KEY_KP_EQUAL, (VTermModifier)mod);
        break;


    default:
        if (keysym >= XKB_KEY_F1 && keysym <= XKB_KEY_F35) {
            VTermKey fsym = (VTermKey)VTERM_KEY_FUNCTION(keysym - XKB_KEY_F1 + 1);
            keyboard_key(fsym, (VTermModifier)mod);
        } else if (mods.ctrl && !mods.alt && !mods.shift && keysym < 127) {
            keyboard_unichar(keysym, (VTermModifier)mod);
        }
        break;
    }
}

VTermWrapper::~VTermWrapper() {
    vterm_free(vterm);
}
