#include "VTermWrapper.h"
#include "SkiaTermEmulator.h"

int VTermWrapper::damage(int start_row, int start_col, int end_row, int end_col) {
    frontend.lock()->refreshRect(start_row, start_col, end_row, end_col);
    return 0;
}

int VTermWrapper::moverect(VTermRect dest, VTermRect src) {
    return 0;
}

int VTermWrapper::movecursor(VTermPos pos, VTermPos oldpos, int visible) {
    // Issuing repainting for the old cursor position
    frontend.lock()->refreshRect(cursor_pos.row, cursor_pos.col, cursor_pos.row + 1, cursor_pos.col + 1);

    cursor_pos = pos;
    
    // Issuing repainting for the new cursor position
    frontend.lock()->refreshRect(cursor_pos.row, cursor_pos.col, cursor_pos.row + 1, cursor_pos.col + 1);
    return 0;
}
int VTermWrapper::settermprop(VTermProp prop, VTermValue *val) {
    switch (prop) {
    case VTERM_PROP_CURSORVISIBLE:
        //this->cursorVisible = val->boolean;
        frontend.lock()->setCursorVisible(val->boolean);
        return true;
        break;
    case VTERM_PROP_CURSORBLINK:
        //this->cursorBlink = val->boolean;
        frontend.lock()->setCursorBlink(val->boolean);
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
    frontend.lock()->bellBlink();
    return 0;
}
int VTermWrapper::resize(int rows, int cols) {
    return 0;
}

int VTermWrapper::sb_pushline(int cols, const VTermScreenCell *cells) {
    return 0;
}

int VTermWrapper::sb_popline(int cols, VTermScreenCell *cells) {
    return 0;
}

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

VTermScreenCellWrapper VTermWrapper::getCell(int32_t row, int32_t col) {
    VTermPos pos = { row, col };
    VTermScreenCell cell;
    vterm_screen_get_cell(screen, pos, &cell);

    VTermScreenCellWrapper res;
    res.width = cell.width;
    res.attrs = cell.attrs;

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

    //if (cell.chars[0] != 0xffffffff) {
        for (int i = 0; cell.chars[i] != 0 && i < VTERM_MAX_CHARS_PER_CELL; i++) {
            res.chars.push_back(cell.chars[i]);
        }
    //} else {
    //    res.chars.push_back(0xffffffff);
    //}
    return res;
}


void VTermWrapper::output_callback(const char* s, size_t len, void* user)
{
    TermSubprocess* subp = reinterpret_cast<TermSubprocess*>(user);
    std::string str(s, len);
    subp->write(str);
}

void VTermWrapper::processInputFromSubprocess() {
    subprocess->readInputAndProcess([&](const std::string& input_str) {
        input_write(input_str.data(), input_str.size());
    });
}

VTermWrapper::VTermWrapper(uint32_t _rows, uint32_t _cols, 
    std::shared_ptr<TermSubprocess> subprocess,
    std::weak_ptr<SkiaTermEmulator> frontend)
        : subprocess(subprocess), frontend(frontend), rows(_rows), cols(_cols) {
    
    vterm = vterm_new(_rows, _cols);
    vterm_set_utf8(vterm, 1);
    vterm_output_set_callback(vterm, output_callback, (void*)subprocess.get());

    screen = vterm_obtain_screen(vterm);
    vterm_screen_set_callbacks(screen, &screen_callbacks, this);

    vterm_screen_enable_altscreen(screen, 1);

    VTermState * state = vterm_obtain_state(vterm);
    
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
