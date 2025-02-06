#include "process_tools.h"

// libpurist headers
#include <purist/graphics/skia/DisplayContentsSkia.h>
#include <purist/graphics/Display.h>
#include <purist/graphics/Mode.h>
#include <purist/input/KeyboardHandler.h>
#include <purist/Platform.h>
#include <purist/graphics/skia/icu_common.h>
#include <Resource.h>

// Skia headers
#include "include/core/SkScalar.h"
#include "include/core/SkImage.h"
#include "include/core/SkRect.h"
#include "include/core/SkSurface.h"
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

// System headers
#include <termios.h>
#include <unistd.h>

// std headers
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


namespace p = purist;
namespace pg = purist::graphics;
namespace pi = purist::input;
namespace pgs = purist::graphics::skia;

template<typename key_t, typename value_t>
class lru_cache {
public:
    typedef typename std::pair<key_t, value_t> key_value_pair_t;
    typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

    lru_cache(size_t max_size) :
        _max_size(max_size) {
    }
    
    void put(const key_t& key, const value_t& value) {
        auto it = _cache_items_map.find(key);
        _cache_items_list.push_front(key_value_pair_t(key, value));
        if (it != _cache_items_map.end()) {
            _cache_items_list.erase(it->second);
            _cache_items_map.erase(it);
        }
        _cache_items_map[key] = _cache_items_list.begin();
        
        if (_cache_items_map.size() > _max_size) {
            auto last = _cache_items_list.end();
            last--;
            _cache_items_map.erase(last->first);
            _cache_items_list.pop_back();
        }
    }
    
    const value_t& get(const key_t& key) {
        auto it = _cache_items_map.find(key);
        if (it == _cache_items_map.end()) {
            throw std::range_error("There is no such key in cache");
        } else {
            _cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
            return it->second->second;
        }
    }
    
    bool exists(const key_t& key) const {
        return _cache_items_map.find(key) != _cache_items_map.end();
    }
    
    size_t size() const {
        return _cache_items_map.size();
    }
    
private:
    std::list<key_value_pair_t> _cache_items_list;
    std::map<key_t, list_iterator_t> _cache_items_map;
    size_t _max_size;
};

template <typename T> class Matrix {
    T* buf;
    int rows, cols;
public:
    Matrix(int _rows, int _cols) : rows(_rows), cols(_cols) {
        buf = new T[cols * rows];
    }
    ~Matrix() {
        delete buf;
    }
    void fill(const T& by) {
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                buf[cols * row + col] = by;
            }
        }
    }
    T& operator()(int row, int col) {
        if (row < 0 || col < 0 || row >= rows || col >= cols) throw std::runtime_error("invalid position");
        //else
        return buf[cols * row + col];
    }
    int getRows() const { return rows; }
    int getCols() const { return cols; }
};

class TermDisplayContents : public pgs::SkiaDisplayContentsHandler, public pi::KeyboardHandler {
public:
    std::weak_ptr<p::Platform> platform;
    float cursorPhase = 0;
    uint32_t cursorBlinkPeriodMSec = 300;

    std::mutex matrixMutex;

    VTerm* vterm;
    VTermScreen* screen;
    //SDL_Surface* surface = NULL;
    //SDL_Texture* texture = NULL;
    int fd;
    pid_t pid;
    //Matrix<unsigned char> matrix;
    //TTF_Font* font;
    std::shared_ptr<SkFont> font;
    SkScalar font_width;
    SkScalar font_height;
    SkScalar font_descent;
    uint32_t ringingFramebuffers = 0;
    bool cursorVisible = true;

    uint32_t rows, cols;

    struct litera_key {
        std::string utf8;
        int width, height;
        SkColor fgcolor, bgcolor;

        bool operator == (const litera_key& other) const {
            return utf8 == other.utf8 &&
                   width == other.width &&
                   height == other.height &&
                   fgcolor == other.fgcolor &&
                   bgcolor == other.bgcolor;
        }

        bool operator < (const litera_key& other) const {
            if (utf8 < other.utf8) return true;
            else if (utf8 == other.utf8) {

                if (width < other.width) return true;
                else if (width == other.width) {

                    if (height < other.height) return true;
                    else if (height == other.height) {

                        if (fgcolor < other.fgcolor) return true;
                        else if (fgcolor == other.fgcolor) {

                            if (bgcolor < other.bgcolor) return true;
                        }
                    }
                }
            }
            return false;                   
        }
    };

    struct row_key : public std::vector<litera_key> {
        bool operator < (const row_key& other) const {
            if (this->size() != other.size()) {
                throw std::logic_error(std::string("Incomparable keys - different length: ") + std::to_string(this->size()) + " and " +  std::to_string(other.size()));
            }
            for (size_t i = 0; i < size(); i++) {
                if ((*this)[i] < other[i]) return true;
                if (!((*this)[i] == other[i])) {
                    return false;
                }
            }
            return false;
        }
    };

    int divider = 4;
    lru_cache<row_key, sk_sp<SkImage>> typesettingBox;
    std::map<uint32_t, std::shared_ptr<Matrix<unsigned char>>> screenUpdateMatrices;  // The key is the display connector id
    

    const VTermScreenCallbacks screen_callbacks = {
        damage,
        moverect,
        movecursor,
        settermprop,
        bell,
        resize,
        sb_pushline,
        sb_popline
    };

    VTermPos cursor_pos;
    uint32_t framebuffersCount = 0;

    VTermColor color_palette[16];

    void setEqualizedColorPalette(VTermState* state) {
        VTermColor black;
        vterm_color_rgb(&black, 1, 1, 1);
        VTermColor red;
        vterm_color_rgb(&red, 128, 0, 0);
        VTermColor green;
        vterm_color_rgb(&green, 0, 96, 0);
        VTermColor yellow;
        vterm_color_rgb(&yellow, 128, 64, 0);
        VTermColor blue;
        vterm_color_rgb(&blue, 24, 24, 255);
        VTermColor magenta;
        vterm_color_rgb(&magenta, 128, 0, 128);
        VTermColor cyan;
        vterm_color_rgb(&cyan, 0, 96, 128);
        VTermColor light_gray;
        vterm_color_rgb(&light_gray, 128, 128, 128);

        VTermColor dark_gray;
        vterm_color_rgb(&dark_gray, 64, 64, 64);
        VTermColor lred;
        vterm_color_rgb(&lred, 255, 48, 48);
        VTermColor lgreen;
        vterm_color_rgb(&lgreen, 48, 224, 48);
        VTermColor lyellow;
        vterm_color_rgb(&lyellow, 192, 192, 32);
        VTermColor lblue;
        vterm_color_rgb(&lblue, 96, 96, 255);
        VTermColor lmagenta;
        vterm_color_rgb(&lmagenta, 255, 64, 255);
        VTermColor lcyan;
        vterm_color_rgb(&lcyan, 64, 192, 224);
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

     void setStandardColorPalette(VTermState* state) {
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

    TermDisplayContents(std::weak_ptr<p::Platform> platform, uint32_t _rows, uint32_t _cols) 
            : platform(platform), rows(_rows), cols(_cols), typesettingBox(_rows * divider * 6) { //4*54*2) {

        // Checking the arguments
        if (_cols == 0 || _rows == 0) {
            throw std::logic_error(std::string("Columns and rows count hade to be fositive"));
        }
        if (_cols % divider != 0) { 
            throw std::logic_error(std::string("Columns count ") + std::to_string(_cols) + std::string(" should be divideable by te number of substrings ") + std::to_string(divider));
        }

        auto prog = getenv("SHELL");
        auto subprocess = createSubprocessWithPty(_rows, _cols, prog, {"-"});
        this->pid = subprocess.first;
        fd = subprocess.second;

        vterm = vterm_new(_rows, _cols);
        vterm_set_utf8(vterm, 1);
        vterm_output_set_callback(vterm, output_callback, (void*)&fd);

        screen = vterm_obtain_screen(vterm);
        vterm_screen_set_callbacks(screen, &screen_callbacks, this);

        VTermState * state = vterm_obtain_state(vterm);
        
        //setEqualizedColorPalette(state);
        setStandardColorPalette(state);

        vterm_screen_set_default_colors(screen, &color_palette[7], &color_palette[0]);


        vterm_screen_reset(screen, 1);
        
        
        //matrix.fill(0);
    }

    void keyboard_unichar(uint32_t c, VTermModifier mod) {
        vterm_keyboard_unichar(vterm, c, mod);
    }
    void keyboard_key(VTermKey key, VTermModifier mod) {
        vterm_keyboard_key(vterm, key, mod);
    }
    void input_write(const char* bytes, size_t len) {
        vterm_input_write(vterm, bytes, len);
    }
    int damage(int start_row, int start_col, int end_row, int end_col) {
        std::lock_guard<std::mutex> lock(matrixMutex);
        //invalidateTexture();
        for (auto& mat_pair : screenUpdateMatrices) {
            auto& matrix = *(mat_pair.second);
            for (int row = start_row; row < end_row; row++) {
                for (int col = start_col; col < end_col; col++) {
                    matrix(row, col) = framebuffersCount;
                }
            }
        }
        return 0;
    }
    int moverect(VTermRect dest, VTermRect src) {
        return 0;
    }
    int movecursor(VTermPos pos, VTermPos oldpos, int visible) {
        // Issuing repainting for the old cursor position
        for (auto& mat_pair : screenUpdateMatrices) {
            auto& matrix = *(mat_pair.second);
            matrix(cursor_pos.row, cursor_pos.col) = framebuffersCount;
        }

        cursor_pos = pos;
        
        // Issuing repainting for the new cursor position
        for (auto& mat_pair : screenUpdateMatrices) {
            auto& matrix = *(mat_pair.second);
            matrix(cursor_pos.row, cursor_pos.col) = framebuffersCount;
        }
        return 0;
    }
    int settermprop(VTermProp prop, VTermValue *val) {
        switch (prop) {
        case VTERM_PROP_CURSORVISIBLE:
            this->cursorVisible = val->boolean;
            break;
        default:
            break;
        }
        return 0;
    }
    int bell() {
        ringingFramebuffers = framebuffersCount;
        return 0;
    }
    int resize(int rows, int cols) {
        return 0;
    }

    int sb_pushline(int cols, const VTermScreenCell *cells) {
        return 0;
    }

    int sb_popline(int cols, VTermScreenCell *cells) {
        return 0;
    }

    virtual ~TermDisplayContents() {
        vterm_free(vterm);
        //invalidateTexture();
        //SDL_FreeSurface(surface);
    }
    
    std::list<std::shared_ptr<pg::Mode>>::const_iterator chooseMode(std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, const std::list<std::shared_ptr<pg::Mode>>& modes) override {

        if (skiaOverlay->getFontMgr() == nullptr) {
            skiaOverlay->createFontMgr({
                LOAD_RESOURCE(fonts_HackNerdFontMono_HackNerdFontMono_Regular_ttf)    // "Hack Nerd Font Mono"
                //LOAD_RESOURCE(fonts_DejaVuSansMono_DejaVuSansMono_ttf)
            });
        }

        //auto typeface = skiaOverlay->getTypefaceByName("DejaVu Sans Mono");
        auto typeface = skiaOverlay->getTypefaceByName("Hack Nerd Font Mono");
        SkScalar fontSize = 1;
        font = std::make_shared<SkFont>(typeface, fontSize);
	    font->setHinting(SkFontHinting::kNone);

        SkFontMetrics mets;
        /*font_height =*/ font->getMetrics(&mets);

        /*SkRect verticalBoxLineBounds;
        char16_t verticalBoxLineChar = u'│'; //u'\u2503';
	    font->measureText((const void*)&verticalBoxLineChar, 2, SkTextEncoding::kUTF8, &verticalBoxLineBounds);*/

        auto font_height_patch = 0.05;
        auto font_width_patch = -0.01;
        auto font_descent_patch = 0.0;

        font_width = mets.fAvgCharWidth + font_width_patch; // This is the real character width for monospace
        font_height = fontSize + font_height_patch;     // Applying font height patch
        font_descent = mets.fDescent + font_descent_patch;              // Patching the descent for the specific font (here is for Hack)

        // Don't allow screens bigger than UHD
        for (auto m = modes.begin(); m != modes.end(); m++) {
             if ((*m)->getWidth() < 4000 && 
                 (*m)->getHeight() < 4000) return m;
        }
        return modes.begin();
    }

    pgs::DisplayOrientation chooseOrientation(std::shared_ptr<pg::Display> display, std::shared_ptr<pgs::SkiaOverlay> skiaOverlay) override {
        if (display->getMode().getHeight() > display->getMode().getWidth()) {
            // We are assumming that all the displays are horizontally oriented.
            // So if some of them have height > width, let's rotate it
            return pgs::DisplayOrientation::LEFT_VERTICAL;
        } else {
            return pgs::DisplayOrientation::HORIZONTAL;
        }
    }

    sk_sp<SkSurface> letter_surface = nullptr;

    sk_sp<SkImage> drawCells(int col_min, int col_max, int row, //int row_min, int row_max, 
                   int buffer_width, int buffer_height,
                   std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, const icu::Normalizer2* normalizer) {

        SkScalar epsilon = 0.0005f; // This very small value is added to the skale factor 
                                   // to make sure that the images will not have any gaps between them

        SkScalar kx = ((SkScalar)buffer_width / (col_max - col_min) + epsilon) / font_width;
        SkScalar ky = ((SkScalar)buffer_height + epsilon) / font_height;

        // SkScalar font_width_scaled = font_width * kx;
        // SkScalar font_height_scaled = font_height * kx;

        SkColor4f color, bgcolor;
        UErrorCode status = U_ZERO_ERROR;

        sk_sp<SkImage> letter_image = nullptr;
        
        /*if (!texture)*/ {
            /*for (int row = row_min; row < row_max; row++)*/ {
                row_key rk;
                for (int col = col_min; col < col_max; col++) {
                    std::string utf8 = "";
                    VTermPos pos = { row, col };
                    VTermScreenCell cell;
                    vterm_screen_get_cell(screen, pos, &cell);
                    if (cell.chars[0] != 0xffffffff) {
                        icu::UnicodeString ustr;
                        for (int i = 0; cell.chars[i] != 0 && i < VTERM_MAX_CHARS_PER_CELL; i++) {
                            ustr.append((UChar32)cell.chars[i]);
                        }
                        //SDL_Color color = (SDL_Color){128,128,128};
                        color = SkColor4f::FromColor(SkColorSetRGB(128, 128, 128));

                        //SDL_Color bgcolor = (SDL_Color){0,0,0};
                        bgcolor = SkColor4f::FromColor(SkColorSetRGB(0, 0, 0));

                        if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
                            vterm_screen_convert_color_to_rgb(screen, &cell.fg);
                        }
                        if (VTERM_COLOR_IS_RGB(&cell.fg)) {
                            // TODO color = (SDL_Color){cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue};
                            color = SkColor4f::FromColor(SkColorSetRGB(cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue));
                        }
                        if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
                            vterm_screen_convert_color_to_rgb(screen, &cell.bg);
                        }
                        if (VTERM_COLOR_IS_RGB(&cell.bg)) {
                            // TODO bgcolor = (SDL_Color){cell.bg.rgb.red, cell.bg.rgb.green, cell.bg.rgb.blue};
                            bgcolor = SkColor4f::FromColor(SkColorSetRGB(cell.bg.rgb.red, cell.bg.rgb.green, cell.bg.rgb.blue));
                        }

                        if (cell.attrs.reverse) std::swap(color, bgcolor);
                        
                        /* TODO
                        int style = TTF_STYLE_NORMAL;
                        if (cell.attrs.bold) style |= TTF_STYLE_BOLD;
                        if (cell.attrs.underline) style |= TTF_STYLE_UNDERLINE;
                        if (cell.attrs.italic) style |= TTF_STYLE_ITALIC;
                        if (cell.attrs.strike) style |= TTF_STYLE_STRIKETHROUGH; */
                        if (cell.attrs.blink) { /*TBD*/ }

                        //SDL_Rect rect = { col * font_width, row * font_height, font_width * cell.width, font_height };

                        
                        // SkPaint bgpt(bgcolor);
                        // bgpt.setStyle(SkPaint::kFill_Style);
                        // canvas.drawRect(rect, bgpt);

                        if (ustr.length() > 0) {
                            auto ustr_normalized = normalizer->normalize(ustr, status);
                            if (U_SUCCESS(status)) {
                                ustr_normalized.toUTF8String(utf8);
                            } else {
                                ustr.toUTF8String(utf8);
                            }

                        }
                    }

                    litera_key litkey { utf8, buffer_width, buffer_height, color.toSkColor(), bgcolor.toSkColor() };
                    rk.push_back(litkey);
                }

                if (!typesettingBox.exists(rk)) {
                    //static int cache_misses = 0;
                    //std::cout << "cache miss: " << cache_misses++ << std::endl;
                    //if (letter_surface == nullptr) {
                        letter_surface = skiaOverlay->getSkiaSurface()->makeSurface(
                            SkImageInfo::MakeN32Premul(((uint32_t)buffer_width) ,   // * (col_max - col_min)
                                                                ((uint32_t)buffer_height))
                        );
                    //}

                    auto& letter_canvas = *letter_surface->getCanvas();

                    letter_canvas.scale(kx, ky);

                    for (int col = col_min; col < col_max; col++) {
                        int c = col - col_min;
                        auto& letter_key = rk[c];

                        SkRect bgrect = { 
                            (float)c * font_width, 
                            0.0f, 
                            (float)(c + 1) * font_width,
                            (float)font_height
                        };
                        SkPaint bgpt(SkColor4f::FromColor(letter_key.bgcolor));
                        bgpt.setStyle(SkPaint::kFill_Style);
                        letter_canvas.drawRect(bgrect, bgpt);
                    }

                    for (int col = col_min; col < col_max; col++) {
                        int c = col - col_min;
                        auto& letter_key = rk[c];

                        // TODO TTF_SetFontStyle(font, style);
                        //std::cout << utf8.c_str();
                        auto& utf8 = letter_key.utf8;

                        SkRect bgrect = { 
                            (float)c * font_width, 
                            0.0f, 
                            (float)(c + 1) * font_width,
                            (float)font_height
                        };
                        letter_canvas.save();
                        letter_canvas.clipRect(bgrect);
                        letter_canvas.drawString(utf8.c_str(),
                                                 c * font_width, font_height - font_descent, *font, 
                                                 SkPaint(SkColor4f::FromColor(letter_key.fgcolor)));
                        letter_canvas.restore();
                    }
                    letter_image = letter_surface->makeImageSnapshot();

                    typesettingBox.put(rk, letter_image);
                } else {
                    letter_image = typesettingBox.get(rk);
                }
            }
        }
        return letter_image;
    }

    void drawIntoSurface(std::shared_ptr<pg::Display> display, std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, int width, int height, SkCanvas& canvas) override {
        processInput();
        if (framebuffersCount < display->getFramebuffersCount()) {
            // Setting how many framebuffers we should redraw at once
            framebuffersCount =  display->getFramebuffersCount();
        }

        if (screenUpdateMatrices.find(display->getConnectorId()) == screenUpdateMatrices.end()) {
            screenUpdateMatrices[display->getConnectorId()] = std::make_shared<Matrix<unsigned char>>(rows, cols);
            screenUpdateMatrices[display->getConnectorId()]->fill(framebuffersCount);
        }
        
        auto& matrix = *(screenUpdateMatrices[display->getConnectorId()]);
        
        SkScalar w = width, h = height;

        // Scale coeffitcients
        SkScalar font_width_scaled = w / matrix.getCols(); //font_width / kx;
        //SkScalar font_height_scaled = h / matrix.getRows(); //font_height / ky;

        // SkScalar kx = (float)(font_width_scaled) / (font_width);
        // SkScalar ky = (float)(font_height_scaled) / (font_height);


        //SkScalar font_descent_scaled = font_descent / ky;

        //canvas.scale(kx, ky);

        std::pair<pid_t, int> rst;
        rst = waitpid(this->pid, WNOHANG);
        if (rst.first == pid) {
            //BREAKING!!!
            throw std::runtime_error("rst.first == pid");
        }

        const SkScalar gray_hsv[] { 0.0f, 0.0f, 0.7f };
        auto color = SkColor4f::FromColor(SkHSVToColor(255, gray_hsv));
        //auto paint_gray = SkPaint(color_gray);

        //const SkScalar black_hsv[] { 0.0f, 0.0f, 0.0f };
        //auto bgcolor = SkColor4f::FromColor(SkHSVToColor(255, black_hsv));
        //auto paint_black = SkPaint(color_black);


        //canvas.clear(bgcolor);

        ///////////////

        // SDL_Event ev;
        // while(SDL_PollEvent(&ev)) {
        //     if (ev.type == SDL_QUIT || (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE && (ev.key.keysym.mod & KMOD_CTRL))) {
        //         kill(pid, SIGTERM);
        //     } else {
        //         terminal.processEvent(ev);
        //     }
        // }

        // auto fullscreenSurface = skiaOverlay->getSkiaSurface()->makeSurface(
        //                 SkImageInfo::MakeN32Premul(font_width_scaled * matrix.getCols(), font_height_scaled * matrix.getRows())
        //             );

        // auto& fullscreenCanvas = *fullscreenSurface->getCanvas();

        // sk_sp<SkImage> oldFullscreenImage = nullptr;
        // auto cachedFullImageKey = this->fullScreen.find(display->getConnectorId());
        // if (cachedFullImageKey != this->fullScreen.end()) {
        //     oldFullscreenImage = cachedFullImageKey->second;
        //     fullscreenCanvas.drawImage(oldFullscreenImage, 0, 0);
        // }

        assert(matrix.getCols() % divider == 0);
        int part_width = matrix.getCols() / divider;

        UErrorCode status = U_ZERO_ERROR;
        auto normalizer = icu::Normalizer2::getNFKCInstance(status);
        if (U_FAILURE(status)) throw std::runtime_error("unable to get NFKC normalizer");

        int buffer_width = w / divider;
        int cols_per_buffer = cols / divider;

        // Patching buffer_width so that a single character consists of an integer number of pixels
        int rem = buffer_width % cols_per_buffer;
        float hscale = 1.0f;
        if (rem > 0) {
            buffer_width += cols_per_buffer - rem;
            hscale = (w / divider) / (float)buffer_width;
        }

        int buffer_height = h / matrix.getRows();
        
        //canvas.scale(hscale, 1.0f);
        
        {
            std::lock_guard<std::mutex> lock(matrixMutex);
            for (int col_part = 0; col_part < divider; col_part++) {
                for (int row = 0; row < matrix.getRows(); row++) {
                    // Checking for the damage
                    bool damaged = false;
                    
                    for (int col = part_width * col_part; col < part_width * (col_part + 1); col++) {
                        if (matrix(row, col) > 0) { matrix(row, col) -= 1; damaged = true; }
                        
                        // Because the cursor is blinking, we are always repainting it
                        if (col == cursor_pos.col && row == cursor_pos.row) { damaged = true; }
                    }

                    //if (damaged) std::cout << "damaged" << std::endl;

                    if (damaged) {
                        // Drawing
                        auto cells_image = drawCells(part_width * col_part,
                                part_width * (col_part + 1), row, 
                                buffer_width, buffer_height, skiaOverlay, normalizer);

                        //canvas.drawImage(cells_image, buffer_width * col_part, buffer_height * row);

                        //void drawImageRect(const SkImage*, const SkRect& dst, const SkSamplingOptions&);
                        SkRect dst = {
                            (float)buffer_width * col_part * hscale, 
                            (float)buffer_height * row,
                            (float)buffer_width * (col_part + 1) * hscale,
                            (float)buffer_height * (row + 1),
                        };
                        SkSamplingOptions so { SkFilterMode::kLinear };
                        canvas.drawImageRect(cells_image, dst, so);
                    }
                }
            }
        }
        
        // draw cursor
        VTermScreenCell cell;
        vterm_screen_get_cell(screen, cursor_pos, &cell);

        auto cur_color = SkColor4f::FromColor(SkColorSetRGB(128, 128, 128));
        if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
            vterm_screen_convert_color_to_rgb(screen, &cell.fg);
        }
        if (VTERM_COLOR_IS_RGB(&cell.fg)) {
            cur_color = SkColor4f::FromColor(SkColorSetRGB(cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue));
        }

        SkRect rect = { 
            (float)cursor_pos.col * font_width_scaled, 
            (float)cursor_pos.row * buffer_height, 
            (float)(cursor_pos.col + 1) * font_width_scaled, 
            (float)(cursor_pos.row + 1) * buffer_height
        };

        if (cursorVisible) {
            auto cur_time = std::chrono::system_clock::now();
            auto sec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(cur_time.time_since_epoch());
            cursorPhase = (float)(sec_since_epoch.count() % cursorBlinkPeriodMSec) / cursorBlinkPeriodMSec;
            float cursor_alpha = 0.5 * sin(2 * M_PI * (float)cursorPhase) + 0.5;
            auto cursor_color = SkColor4f({ 
                cur_color.fR,
                cur_color.fG,
                cur_color.fB,
                cursor_alpha });
            auto cursor_paint = SkPaint(cursor_color);

            //SkPaint cur_paint(cur_color);
            cursor_paint.setStyle(SkPaint::kFill_Style);
            canvas.drawRect(rect, cursor_paint);
        }

        if (ringingFramebuffers) {
            canvas.clear(color);
            for (auto& mat_pair : screenUpdateMatrices) {
                auto& matrix = *(mat_pair.second);
                matrix.fill(framebuffersCount);
            }
            ringingFramebuffers -= 1;
        }
    }

    void onCharacter(pi::Keyboard& kbd, char32_t charCode, pi::Modifiers mods, pi::Leds leds) override { 
        
        // TODO const Uint8 *state = SDL_GetKeyboardState(NULL);
        // int mod = VTERM_MOD_NONE;
        // if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL]) mod |= VTERM_MOD_CTRL;
        // if (state[SDL_SCANCODE_LALT] || state[SDL_SCANCODE_RALT]) mod |= VTERM_MOD_ALT;
        // if (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT]) mod |= VTERM_MOD_SHIFT;
        
        int mod = VTERM_MOD_NONE;
        if (mods.ctrl) mod |= VTERM_MOD_CTRL;
        if (mods.alt) mod |= VTERM_MOD_ALT;
        if (mods.shift) mod |= VTERM_MOD_SHIFT;


        //for (int i = 0; i < strlen(ev.text.text); i++) {
            keyboard_unichar(charCode, (VTermModifier)mod);   //ev.text.text[i], (VTermModifier)mod);
        //}

    }
    void onKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat) override { 
        /*if (keysym == XKB_KEY_Escape) {
            platform.lock()->stop();
        } else {
            textInput.onKeyPress(kbd, keysym, mods, leds, repeat);
        }*/

        //if (repeat) return;

        int mod = VTERM_MOD_NONE;
        if (mods.ctrl) mod |= VTERM_MOD_CTRL;
        if (mods.alt) mod |= VTERM_MOD_ALT;
        if (mods.shift) mod |= VTERM_MOD_SHIFT;


        switch (keysym) {
        case XKB_KEY_Return:
        case XKB_KEY_KP_Enter:
            keyboard_key(VTERM_KEY_ENTER, (VTermModifier)mod);
            break;
        case XKB_KEY_BackSpace:
            keyboard_key(VTERM_KEY_BACKSPACE, (VTermModifier)mod);
            break;
        case XKB_KEY_Escape:
            keyboard_key(VTERM_KEY_ESCAPE, (VTermModifier)mod);
            break;
        case XKB_KEY_Tab:
            keyboard_key(VTERM_KEY_TAB, (VTermModifier)mod);
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
        case XKB_KEY_Page_Up:
        case XKB_KEY_KP_Page_Up:
            keyboard_key(VTERM_KEY_PAGEUP, (VTermModifier)mod);
            break;
        case XKB_KEY_Page_Down:
        case XKB_KEY_KP_Page_Down:
            keyboard_key(VTERM_KEY_PAGEDOWN, (VTermModifier)mod);
            break;
        case XKB_KEY_Home:
        case XKB_KEY_KP_Home:
            keyboard_key(VTERM_KEY_HOME, (VTermModifier)mod);
            break;
        case XKB_KEY_End:
        case XKB_KEY_KP_End:
            keyboard_key(VTERM_KEY_END, (VTermModifier)mod);
            break;
        case XKB_KEY_Delete:
        case XKB_KEY_KP_Delete:
            keyboard_key(VTERM_KEY_DEL, (VTermModifier)mod);
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

    void onKeyRelease(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds) override { }

    void processInput() {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeval timeout = { 0, 0 };
        if (select(fd + 1, &readfds, NULL, NULL, &timeout) > 0) {
            char buf[cols * rows * 4]; //4096];
            auto size = read(fd, buf, sizeof(buf));
            if (size > 0) {
                input_write(buf, size);
            }
        }
    }

    static void output_callback(const char* s, size_t len, void* user);
    static int damage(VTermRect rect, void *user);
    static int moverect(VTermRect dest, VTermRect src, void *user);
    static int movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user);
    static int settermprop(VTermProp prop, VTermValue *val, void *user);
    static int bell(void *user);
    static int resize(int rows, int cols, void *user);
    static int sb_pushline(int cols, const VTermScreenCell *cells, void *user);
    static int sb_popline(int cols, VTermScreenCell *cells, void *user);

};

void TermDisplayContents::output_callback(const char* s, size_t len, void* user)
{
    //std::string ss(s, len);
    //std::cout << "> " << ss << std::endl;
    write(*(int*)user, s, len);
}

int TermDisplayContents::damage(VTermRect rect, void *user)
{
    return ((TermDisplayContents*)user)->damage(rect.start_row, rect.start_col, rect.end_row, rect.end_col);
}

int TermDisplayContents::moverect(VTermRect dest, VTermRect src, void *user)
{
    return ((TermDisplayContents*)user)->moverect(dest, src);
}

int TermDisplayContents::movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user)
{
    return ((TermDisplayContents*)user)->movecursor(pos, oldpos, visible);
}

int TermDisplayContents::settermprop(VTermProp prop, VTermValue *val, void *user)
{
    return ((TermDisplayContents*)user)->settermprop(prop, val);
}

int TermDisplayContents::bell(void *user)
{
    return ((TermDisplayContents*)user)->bell();
}

int TermDisplayContents::resize(int rows, int cols, void *user)
{
    return ((TermDisplayContents*)user)->resize(rows, cols);
}

int TermDisplayContents::sb_pushline(int cols, const VTermScreenCell *cells, void *user)
{
    return ((TermDisplayContents*)user)->sb_pushline(cols, cells);
}

int TermDisplayContents::sb_popline(int cols, VTermScreenCell *cells, void *user)
{
   return ((TermDisplayContents*)user)->sb_popline(cols, cells);
}


int main(int argc, char **argv)
{
    try {
        bool enableOpenGL = true;

        auto purist = std::make_shared<p::Platform>(enableOpenGL);

        //const int rows = 24, cols = 80;    // Tiny
        //const int rows = 30, cols = 100;    // Small
        //const int rows = 40, cols = 136;    // Middle
        const int rows = 45, cols = 152;    // Large
        //const int rows = 60, cols = 200;    // Huge


        auto contents = std::make_shared<TermDisplayContents>(purist, rows, cols);
        auto contents_handler_for_skia = std::make_shared<pgs::DisplayContentsHandlerForSkia>(contents, enableOpenGL);

        purist->run(contents_handler_for_skia, contents);

        return 0;

    } catch (const purist::errcode_exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
        return ex.errcode;
    }
}
