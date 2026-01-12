#include "TermSubprocess.h"
#include "lru_cache.h"
#include "cells.h"

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


// For FullHD resolution:
const int rows = 12, cols = 40;       // Toy    (3.33333)
//const int rows = 25, cols = 80;       // Tiny    (3.33333)
//const int rows = 30, cols = 100;    // Small   (3.33333)
//const int rows = 42, cols = 136;    // Middle  (3.4)
//const int rows = 45, cols = 152;    // Large   (3.37777)
//const int rows = 50, cols = 160;    // Larger    (3.2)
//const int rows = 60, cols = 192;    // Huge    (3.2)
//const int rows = 72, cols = 240;    // Gigantic  (3.33333)

int FPS = 60;

class TermDisplayContents : public pgs::SkiaDisplayContentsHandler, public pi::KeyboardHandler {
public:
    std::weak_ptr<p::Platform> platform;
    float cursorPhase = 0;
    uint32_t cursorBlinkPeriodMSec = 300;

    std::mutex matrixMutex;

    VTerm* vterm;
    VTermScreen* screen;
    int fd;
    pid_t pid;
    std::shared_ptr<SkFont> font;
    SkScalar font_width;
    SkScalar font_height;
    SkScalar font_descent;
    uint32_t ringingFramebuffers = 0;
    bool cursorVisible = true;
    bool cursorBlink = true;

    uint32_t rows, cols;

    std::shared_ptr<TermSubprocess> subprocess;

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

    struct row_key : private std::vector<litera_key> {
        int width, height;
        bool operator == (const row_key& other) const {
            return this->width == other.width &&
                   this->height == other.height &&
                   std::equal(this->begin(), this->end(), other.begin(), other.end());
        }

        bool operator < (const row_key& other) const {
            if (this->size() != other.size()) {
                throw std::logic_error(std::string("Incomparable keys - different length: ") + std::to_string(this->size()) + " and " +  std::to_string(other.size()));
            }

            if (this->width < other.width) return true;
            if (this->width > other.width) return false;

            if (this->height < other.height) return true;
            if (this->height > other.height) return false;

            for (size_t i = 0; i < size(); i++) {
                if ((*this)[i] < other[i]) return true;
                if (!((*this)[i] == other[i])) return false;
            }
            return false;
        }

        litera_key& operator [] (size_t i) {
            return std::vector<litera_key>::operator [] (i);
        }

        const litera_key& operator [] (size_t i) const {
            return std::vector<litera_key>::operator [] (i);
        }

        void push_back(const litera_key& lk) {
            std::vector<litera_key>::push_back(lk);
        }
    };

    int divider = 4;
    struct SurfaceAndImage {
        sk_sp<SkSurface> surface;
        sk_sp<SkImage> image;
    };

    lru_cache<row_key, SurfaceAndImage> typesettingBox;
    std::map<uint32_t, std::shared_ptr<cells<unsigned char>>> screenUpdateMatrices;  // The key is the display connector id
    

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
            : platform(platform), rows(_rows), cols(_cols), typesettingBox(_rows * divider * 4) { //4*54*2) {

        // Checking the arguments
        if (_cols == 0 || _rows == 0) {
            throw std::logic_error(std::string("Columns and rows count has to be positive"));
        }
        if (_cols % divider != 0) { 
            throw std::logic_error(std::string("Columns count ") + std::to_string(_cols) + std::string(" should be divideable by te number of substrings ") + std::to_string(divider));
        }

        std::string prog = getenv("SHELL");
        subprocess = std::make_shared<TermSubprocess>(_rows, _cols, prog, std::vector<std::string> {"-"});

        //auto subprocess = createSubprocessWithPty(_rows, _cols, prog, {"-"});

        //this->pid = subprocess.first;
        //fd = subprocess.second;

        vterm = vterm_new(_rows, _cols);
        vterm_set_utf8(vterm, 1);
        vterm_output_set_callback(vterm, output_callback, (void*)subprocess.get());

        screen = vterm_obtain_screen(vterm);
        vterm_screen_set_callbacks(screen, &screen_callbacks, this);

        VTermState * state = vterm_obtain_state(vterm);
        
        //setEqualizedColorPalette(state);
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

    std::shared_ptr<std::thread> inputThread;

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
        //std::lock_guard<std::mutex> lock(input_mutex);
        //input_scroll_slowdown_flag = 1;
        
        //const uint64_t usec_per_frame = 1000000 / FPS;
        //std::this_thread::sleep_for(std::chrono::milliseconds((long)usec_per_frame / 1000));
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
            return true;
            break;
        case VTERM_PROP_CURSORBLINK:
            this->cursorBlink = val->boolean;
            return true;
            break;

        case VTERM_PROP_CURSORSHAPE:
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
        // for (auto m = modes.begin(); m != modes.end(); m++) {
        //      if ((*m)->getWidth() < 4000 &&  
        //          (*m)->getHeight() < 4000) {
        //             return m;
        //          }
        // }
        // return modes.begin();

        std::list<std::shared_ptr<pg::Mode>>::const_iterator res = modes.begin();

        int max_width = 0;
		for(auto mode = modes.begin(); mode != modes.end(); mode++) {
			if ((*mode)->getFreq() == FPS && (*mode)->getWidth() < 2000) {
                if (max_width < (*mode)->getWidth()) {
                    res = mode;
                    max_width = (*mode)->getWidth();
                    std::cout << "max_width: " << max_width << std::endl;
                }
			}
		}

        FPS = (*res)->getFreq();

        return res;
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

    //sk_sp<SkSurface> letter_surface = nullptr;
    std::vector<sk_sp<SkSurface>> letter_surfaces;

    sk_sp<SkImage> drawCells(int col_min, int col_max, int row, //int row_min, int row_max, 
                   int buffer_width, int buffer_height,
                   std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, const icu::Normalizer2* normalizer) {

        SkScalar epsilon = 0.0005f; // This very small value is added to the skale factor 
                                   // to make sure that the images will not have any gaps between them

        SkScalar kx = ((SkScalar)buffer_width / (col_max - col_min) + epsilon) / font_width;
        SkScalar ky = ((SkScalar)buffer_height + epsilon) / font_height;

        SkColor4f color, bgcolor;
        UErrorCode status = U_ZERO_ERROR;

        sk_sp<SkImage> letter_image = nullptr;
        
        /*if (!texture)*/ {
            /*for (int row = row_min; row < row_max; row++)*/ {
                row_key rk;
                rk.width = buffer_width;
                rk.height = buffer_height;
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
                        color = SkColor4f::FromColor(SkColorSetRGB(128, 128, 128));

                        bgcolor = SkColor4f::FromColor(SkColorSetRGB(0, 0, 0));

                        if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
                            vterm_screen_convert_color_to_rgb(screen, &cell.fg);
                        }
                        if (VTERM_COLOR_IS_RGB(&cell.fg)) {
                            color = SkColor4f::FromColor(SkColorSetRGB(cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue));
                        }
                        if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
                            vterm_screen_convert_color_to_rgb(screen, &cell.bg);
                        }
                        if (VTERM_COLOR_IS_RGB(&cell.bg)) {
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
                        
                        if (letter_surfaces.empty()) {
                            letter_surfaces.push_back(skiaOverlay->getSkiaSurface()->makeSurface(
                                SkImageInfo::MakeN32Premul(((uint32_t)buffer_width) ,   // * (col_max - col_min)
                                                                    ((uint32_t)buffer_height))
                            ));
                            std::cout << "Added new letter surface. In the cache: " << typesettingBox.size() << std::endl;
                        }
                        
                        sk_sp<SkSurface> letter_surface = nullptr;
                        letter_surface = letter_surfaces.back();
                        letter_surfaces.pop_back();

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

                    std::optional<std::pair<row_key, SurfaceAndImage>> returned_back = typesettingBox.put(rk, { letter_surface, letter_image });
                    if (returned_back.has_value()) {
                        auto& ret_surf = returned_back.value().second.surface;
                        auto ret_cnv = ret_surf->getCanvas();
                        ret_cnv->restoreToCount(0);
                        ret_cnv->resetMatrix();
                        //ret_cnv->clear(SK_ColorTRANSPARENT);
                        letter_surfaces.push_back(ret_surf);
                    }

                } else {
                    letter_image = typesettingBox.get(rk).image;
                    assert(letter_image->width() == buffer_width && letter_image->height() == buffer_height);
                }
            }
        }
        return letter_image;
    }

    void drawIntoSurface(std::shared_ptr<pg::Display> display, 
                         std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, 
                         int width, int height, SkCanvas& canvas, bool refreshed) override {

        if (subprocess->isExited()) {
            platform.lock()->stop();
            return;
        }
        subprocess->readInputAndProcess([&](const std::string& input_str) {
            input_write(input_str.data(), input_str.size());
        });


        if (framebuffersCount < display->getFramebuffersCount()) {
            // Setting how many framebuffers we should redraw at once
            framebuffersCount = display->getFramebuffersCount();
        }

        if (screenUpdateMatrices.find(display->getConnectorId()) == screenUpdateMatrices.end()) {
            screenUpdateMatrices[display->getConnectorId()] = std::make_shared<cells<unsigned char>>(rows, cols);
            screenUpdateMatrices[display->getConnectorId()]->fill(framebuffersCount);
        }
        
        auto& matrix = *(screenUpdateMatrices[display->getConnectorId()]);
        
        SkScalar w = width, h = height;

        // Scale coeffitcients
        //SkScalar font_height_scaled = h / matrix.getRows(); //font_height / ky;

        // SkScalar kx = (float)(font_width_scaled) / (font_width);
        // SkScalar ky = (float)(font_height_scaled) / (font_height);


        //SkScalar font_descent_scaled = font_descent / ky;

        //canvas.scale(kx, ky);


        const SkScalar gray_hsv[] { 0.0f, 0.0f, 0.7f };
        auto color = SkColor4f::FromColor(SkHSVToColor(255, gray_hsv));
        //auto paint_gray = SkPaint(color_gray);

        const SkScalar black_hsv[] { 0.0f, 0.0f, 0.0f };
        auto bgcolor = SkColor4f::FromColor(SkHSVToColor(255, black_hsv));

        //auto paint_black = SkPaint(bgcolor);
        //canvas.clear(bgcolor);

        ///////////////


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
        bool presize_horizontal = true;
        int rem = buffer_width % cols_per_buffer;
        float hscale = 1.0f;
        if (presize_horizontal && rem > 0) {
             buffer_width += cols_per_buffer - rem;
             hscale = (w / divider) / (float)buffer_width;
        }

        int buffer_height = h / matrix.getRows();

        // Checking if the height is even
        float vscale = 1.0f;
        if (matrix.getRows() * buffer_height == h) {
            vscale = 1.0f; // no scale
        } else {
            // Let the height be a bit over the necessary size, 
            // because shrinking the screen buffer looks beautifullier than stretching
            buffer_height ++;
            vscale = h / (static_cast<float>(buffer_height) * matrix.getRows());
        }
        
        canvas.scale(hscale, vscale);
        
        SkScalar font_width_scaled = w / matrix.getCols() / hscale;

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
                        if (presize_horizontal) {
                            SkRect dst = {
                                (float)buffer_width * col_part, // * hscale, 
                                (float)buffer_height * row,
                                (float)buffer_width * (col_part + 1), // * hscale,
                                (float)buffer_height * (row + 1),
                            };
                            SkSamplingOptions so { SkFilterMode::kLinear };
                            canvas.drawImageRect(cells_image, dst, so);
                        } else {
                            canvas.drawImage(cells_image, buffer_width * col_part, buffer_height * row);
                        }

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
            if (cursorBlink) {
                cursorPhase = (float)(sec_since_epoch.count() % cursorBlinkPeriodMSec) / cursorBlinkPeriodMSec;
            } else {
                cursorPhase = 0.0f;
            }
            float cursor_alpha = 0.5 * cos(2 * M_PI * (float)cursorPhase) + 0.5;
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

        if (ringingFramebuffers > 0) {
            canvas.clear(color);
            for (auto& mat_pair : screenUpdateMatrices) {
                auto& matrix = *(mat_pair.second);
                matrix.fill(framebuffersCount);
            }
            ringingFramebuffers -= 1;
        }
    }

    void onCharacter(pi::Keyboard& kbd, char32_t charCode, pi::Modifiers mods, pi::Leds leds) override { 
                
        int mod = VTERM_MOD_NONE;
        if (mods.ctrl) mod |= VTERM_MOD_CTRL;
        if (mods.alt) mod |= VTERM_MOD_ALT;
        if (mods.shift) mod |= VTERM_MOD_SHIFT;


        //for (int i = 0; i < strlen(ev.text.text); i++) {
            keyboard_unichar(charCode, (VTermModifier)mod);   //ev.text.text[i], (VTermModifier)mod);
        //}

    }
    void onKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat) override { 

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
    
    TermSubprocess* subp = reinterpret_cast<TermSubprocess*>(user);
    std::string str(s, len);
    subp->write(str);
    
    //write(*(int*)user, s, len);
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

        auto contents = std::make_shared<TermDisplayContents>(purist, rows, cols);
        auto contents_handler_for_skia = std::make_shared<pgs::DisplayContentsHandlerForSkia>(contents, enableOpenGL);

        purist->run(contents_handler_for_skia, contents);

        return 0;

    } catch (const purist::errcode_exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
        return ex.errcode;
    }
}
