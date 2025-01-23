// libpurist headers
//#include <purist/graphics/skia/TextInput.h>
#include "vterm_keycodes.h"
#include <purist/graphics/skia/DisplayContentsSkia.h>
#include <purist/graphics/Display.h>
#include <purist/graphics/Mode.h>
#include <purist/input/KeyboardHandler.h>
#include <purist/Platform.h>
#include <purist/graphics/skia/icu_common.h>
#include <Resource.h>

// Skia headers
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

#include <stdexcept>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <vterm.h>
#include <termios.h>
#include <pty.h>
#include <sys/wait.h>

// std headers
#include <map>
#include <memory>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <xkbcommon/xkbcommon-keysyms.h>

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

std::pair<int, int> createSubprocessWithPty(int rows, int cols, const char* prog, const std::vector<std::string>& args = {}, const char* TERM = "xterm-256color")
{
    int fd;
    struct winsize win = { (unsigned short)rows, (unsigned short)cols, 0, 0 };
    auto pid = forkpty(&fd, NULL, NULL, &win);
    if (pid < 0) throw std::runtime_error("forkpty failed");
    //else
    if (!pid) {
        setenv("TERM", TERM, 1);
        char ** argv = new char *[args.size() + 2];
        argv[0] = strdup(prog);
        for (int i = 1; i <= args.size(); i++) {
            argv[i] = strdup(args[i - 1].c_str());
        }
        argv[args.size() + 1] = NULL;
        if (execvp(prog, argv) < 0) exit(-1);
    }
    //else 
    return { pid, fd };
}

std::pair<pid_t,int> waitpid(pid_t pid, int options)
{
    int status;
    auto done_pid = waitpid(pid, &status, options);
    return {done_pid, status};
}


class TermDisplayContents : public pgs::SkiaDisplayContentsHandler, public pi::KeyboardHandler {
public:
	std::weak_ptr<p::Platform> platform;
	uint32_t cursor_phase = 0;
    uint32_t cursor_loop_len = 30;

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
    int font_width;
    int font_height;
    int font_descent;
    uint32_t ringingFramebuffers = 0;

    int rows, cols;

    struct litera_key {
        std::string utf8;
        uint32_t width, height;
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
                throw std::logic_error("Incomparable keys");
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

	TermDisplayContents(std::weak_ptr<p::Platform> platform, int _rows, int _cols) 
			: platform(platform), rows(_rows), cols(_cols), typesettingBox(_rows * divider * 4) { //4*54*2) {

		auto prog = getenv("SHELL");
		auto subprocess = createSubprocessWithPty(_rows, _cols, prog, {"-"});
		this->pid = subprocess.first;
		fd = subprocess.second;

        vterm = vterm_new(_rows,_cols);
        vterm_set_utf8(vterm, 1);
        vterm_output_set_callback(vterm, output_callback, (void*)&fd);

        screen = vterm_obtain_screen(vterm);
        vterm_screen_set_callbacks(screen, &screen_callbacks, this);
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
				LOAD_RESOURCE(fonts_HackNerdFontMono_HackNerdFontMono_Regular_ttf)
			});
		}

		auto typeface = skiaOverlay->getTypefaceByName("Hack Nerd Font Mono");
		font = std::make_shared<SkFont>(typeface, 20);
		
		SkFontMetrics mets;
		/*font_height = */font->getMetrics(&mets);

		font_width = mets.fAvgCharWidth;
        font_descent = mets.fDescent;
		font_height = mets.fBottom - mets.fTop; //mets.fDescent - mets.fAscent;

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

    void drawCells(int col_min, int col_max, int row, //int row_min, int row_max, 
                   uint32_t font_width_scaled, uint32_t font_height_scaled, float kx, float ky,
                   std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, SkCanvas& canvas, const icu::Normalizer2* normalizer) {
        SkColor4f color, bgcolor;
        UErrorCode status = U_ZERO_ERROR;

        sk_sp<SkSurface> letter_surface = nullptr;

        
        /*if (!texture)*/ {
            /*for (int row = row_min; row < row_max; row++)*/ {
                row_key rk;
                for (int col = col_min; col < col_max; col++) {
                    /*if (matrix(row, col))*/ {
                        VTermPos pos = { row, col };
                        VTermScreenCell cell;
                        vterm_screen_get_cell(screen, pos, &cell);
                        if (cell.chars[0] == 0xffffffff) continue;
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

                        std::string utf8;

                        if (ustr.length() > 0) {
                            auto ustr_normalized = normalizer->normalize(ustr, status);
                            if (U_SUCCESS(status)) {
                                ustr_normalized.toUTF8String(utf8);
                            } else {
                                ustr.toUTF8String(utf8);
                            }

                        }

                        litera_key litkey { utf8, font_width_scaled, font_height_scaled, color.toSkColor(), bgcolor.toSkColor() };
                        rk.push_back(litkey);
                    }

                }

                SkRect rect = { 
                    (float)col_min * font_width_scaled, 
                    (float)row * font_height_scaled, 
                    (float)(col_max) * font_width_scaled, 
                    (float)(row + 1) * font_height_scaled, 
                    // (float)font_width * cell.width, 
                    // (float)font_height 
                };


                sk_sp<SkImage> letter_image = nullptr;
                if (!typesettingBox.exists(rk)) {

                    if (letter_surface == nullptr) {
                        letter_surface = skiaOverlay->getSkiaSurface()->makeSurface(
                            SkImageInfo::MakeN32Premul(font_width_scaled * (col_max - col_min),
                                                                font_height_scaled /** (row_max - row_min)*/)
                        );
                    }

                    auto& letter_canvas = *letter_surface->getCanvas();
                    letter_canvas.scale(kx, ky);
                    //letter_canvas.clear(bgcolor);

                    for (int col = col_min; col < col_max; col++) {
                        int c = col - col_min;

                        // TODO TTF_SetFontStyle(font, style);
                        //std::cout << utf8.c_str();
                        auto& letter_key = rk[c];
                        auto& utf8 = letter_key.utf8;

                        SkRect bgrect = { 
                            (float)c * font_width, 
                            0.0f, 
                            (float)(c + 1) * font_width, 
                            (float)font_height, 
                        };
						SkPaint bgpt(SkColor4f::FromColor(letter_key.bgcolor));
						bgpt.setStyle(SkPaint::kFill_Style);
						letter_canvas.drawRect(bgrect, bgpt);

                        letter_canvas.drawString(utf8.c_str(),
                                                 c * font_width, font_height - font_descent, *font, 
                                                 SkPaint(SkColor4f::FromColor(letter_key.fgcolor)));
                        

                        //std::cout << "backed: " << letter_image->isTextureBacked() << std::endl;

                    }
                    letter_image = letter_surface->makeImageSnapshot();

                    typesettingBox.put(rk, letter_image);
                } else {
                    letter_image = typesettingBox.get(rk);
                }
                
                canvas.drawImage(letter_image, rect.left(), rect.top());
            }
            //texture = SDL_CreateTextureFromSurface(renderer, surface);
            //SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        }
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
        uint32_t font_width_scaled = w / matrix.getCols(); //font_width / kx;
        uint32_t font_height_scaled = h / matrix.getRows(); //font_height / ky;

        SkScalar kx = (float)(font_width_scaled) / (font_width);
        SkScalar ky = (float)(font_height_scaled) / (font_height);


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
                        drawCells(part_width * col_part,
                                part_width * (col_part + 1), row, //row + 1, 
                                    font_width_scaled, font_height_scaled, kx, ky, skiaOverlay, canvas, normalizer);
                    }
                }
            }
        }

        // auto newFullscreenImage = fullscreenSurface->makeImageSnapshot();
        // canvas.drawImage(newFullscreenImage, 0, 0);
        // this->fullScreen[display->getConnectorId()] = newFullscreenImage;

        //SDL_RenderCopy(renderer, texture, NULL, &window_rect);
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

        //SDL_Rect rect = { cursor_pos.col * font_width, cursor_pos.row * font_height, font_width, font_height };
		SkRect rect = { 
			(float)cursor_pos.col * font_width_scaled, 
			(float)cursor_pos.row * font_height_scaled, 
			(float)(cursor_pos.col + 1) * font_width_scaled, 
			(float)(cursor_pos.row + 1) * font_height_scaled
		};
        
		// scale cursor
        // rect.x = window_rect.x + rect.x * window_rect.w / surface->w;
        // rect.y = window_rect.y + rect.y * window_rect.h / surface->h;
        // rect.w = rect.w * window_rect.w / surface->w;
        // rect.w *= cell.width;
        // rect.h = rect.h * window_rect.h / surface->h;
        // SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        // SDL_SetRenderDrawColor(renderer, 255,255,255,96 );
        // SDL_RenderFillRect(renderer, &rect);
        // SDL_SetRenderDrawColor(renderer, 255,255,255,255 );
        // SDL_RenderDrawRect(renderer, &rect);

        cursor_phase = (cursor_phase + 1) % cursor_loop_len;
        float cursor_alpha = 0.5 * sin(2 * M_PI * (float)cursor_phase / cursor_loop_len) + 0.5;
        auto cursor_color = SkColor4f({ 
            cur_color.fR,
            cur_color.fG,
            cur_color.fB,
            cursor_alpha });
        auto cursor_paint = SkPaint(cursor_color);

		//SkPaint cur_paint(cur_color);
		cursor_paint.setStyle(SkPaint::kFill_Style);
		canvas.drawRect(rect, cursor_paint);

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
            char buf[4096];
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
		
		// cols: 320

		const int rows = 40, cols = 160;  // ---- middle, works for HD, FullHD
		//const int rows = 72, cols = 320;  // ---- huge, only for 4K
        

		auto contents = std::make_shared<TermDisplayContents>(purist, rows, cols);
		auto contents_handler_for_skia = std::make_shared<pgs::DisplayContentsHandlerForSkia>(contents, enableOpenGL);
		
		purist->run(contents_handler_for_skia, contents);

		return 0;
	
	} catch (const purist::errcode_exception& ex) {
		fprintf(stderr, "%s\n", ex.what());
		return ex.errcode;
	}
}
