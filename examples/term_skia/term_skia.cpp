// libpurist headers
//#include <purist/graphics/skia/TextInput.h>
#include "include/core/SkImage.h"
#include "include/core/SkRect.h"
#include "include/core/SkSurface.h"
#include "vterm_keycodes.h"
#include <purist/graphics/skia/DisplayContentsSkia.h>
#include <purist/graphics/Display.h>
#include <purist/graphics/Mode.h>
#include <purist/input/KeyboardHandler.h>
#include <purist/Platform.h>
#include <purist/graphics/skia/icu_common.h>

#include <Resource.h>

// Skia headers
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

	VTerm* vterm;
    VTermScreen* screen;
    //SDL_Surface* surface = NULL;
    //SDL_Texture* texture = NULL;
    int fd;
	pid_t pid;
    Matrix<unsigned char> matrix;
    //TTF_Font* font;
	std::shared_ptr<SkFont> font;
    int font_width;
    int font_height;
    int font_descent;
    bool ringing = false;

    struct litera_key {
        std::string utf8;
        uint32_t width, height;
        SkColor fgcolor, bgcolor;

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

    std::map<litera_key, sk_sp<SkImage>> typesettingBox;

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

	TermDisplayContents(std::weak_ptr<p::Platform> platform, int _rows, int _cols) 
			: platform(platform), matrix(_rows, _cols) {

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

        matrix.fill(0);
        //TTF_SizeUTF8(font, "X", &font_width, NULL);
        //surface = SDL_CreateRGBSurfaceWithFormat(0, font_width * _cols, font_height * _rows, 32, SDL_PIXELFORMAT_RGBA32);
        
        //SDL_CreateRGBSurface(0, font_width, font_height, 32, 0, 0, 0, 0);
        //SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
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
        //invalidateTexture();
        for (int row = start_row; row < end_row; row++) {
            for (int col = start_col; col < end_col; col++) {
                matrix(row, col) = 1;
            }
        }
        return 0;
    }
    int moverect(VTermRect dest, VTermRect src) {
        return 0;
    }
    int movecursor(VTermPos pos, VTermPos oldpos, int visible) {
        cursor_pos = pos;
        return 0;
    }
    int settermprop(VTermProp prop, VTermValue *val) {
        return 0;
    }
    int bell() {
        ringing = true;
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

    void drawIntoSurface(std::shared_ptr<pg::Display> display, std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, int width, int height, SkCanvas& canvas) override {
		processInput();
        
		SkScalar w = width, h = height;

        // Scale coeffitcients
        SkScalar kx = (float)w / (font_width * matrix.getCols());
        SkScalar ky = (float)h / (font_height * matrix.getRows());

        uint32_t font_width_scaled = w / matrix.getCols(); //font_width / kx;
        uint32_t font_height_scaled = h / matrix.getRows(); //font_height / ky;
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

		const SkScalar black_hsv[] { 0.0f, 0.0f, 0.0f };
		auto bgcolor = SkColor4f::FromColor(SkHSVToColor(255, black_hsv));
		//auto paint_black = SkPaint(color_black);


		canvas.clear(bgcolor);

		///////////////

		// SDL_Event ev;
        // while(SDL_PollEvent(&ev)) {
        //     if (ev.type == SDL_QUIT || (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE && (ev.key.keysym.mod & KMOD_CTRL))) {
        //         kill(pid, SIGTERM);
        //     } else {
        //         terminal.processEvent(ev);
        //     }
        // }

        UErrorCode status = U_ZERO_ERROR;
        auto normalizer = icu::Normalizer2::getNFKCInstance(status);
        if (U_FAILURE(status)) throw std::runtime_error("unable to get NFKC normalizer");

        /*if (!texture)*/ {
            for (int row = 0; row < matrix.getRows(); row++) {
                for (int col = 0; col < matrix.getCols(); col++) {
                    //if (matrix(row, col)) {
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
						SkRect rect = { 
							(float)col * font_width_scaled, 
							(float)row * font_height_scaled, 
							(float)(col+1) * font_width_scaled, 
							(float)(row+1) * font_height_scaled, 
							// (float)font_width * cell.width, 
							// (float)font_height 
						};
                        
						// SkPaint bgpt(bgcolor);
						// bgpt.setStyle(SkPaint::kFill_Style);
						// canvas.drawRect(rect, bgpt);

                        if (ustr.length() > 0) {
                            auto ustr_normalized = normalizer->normalize(ustr, status);
                            std::string utf8;
                            if (U_SUCCESS(status)) {
                                ustr_normalized.toUTF8String(utf8);
                            } else {
                                ustr.toUTF8String(utf8);
                            }

                            litera_key litkey { utf8, font_width_scaled, font_height_scaled, color.toSkColor(), bgcolor.toSkColor() };
                            auto literaInBox = typesettingBox.find(litkey);
                            sk_sp<SkImage> letter_image = nullptr;
                            if (literaInBox == typesettingBox.end()) {
                                //SkSurfaces::WrapBackendTexture
                                //SkImage::makeTextureImage

                                auto letter_surface = skiaOverlay->getSkiaSurface()->makeSurface(SkImageInfo::MakeN32Premul(font_width_scaled, font_height_scaled));
                                //auto letter_surface = skiaOverlay->getSkiaSurface()->makeSurface(font_width_scaled, font_height_scaled);
                                auto& letter_canvas = *letter_surface->getCanvas();

                                // TODO TTF_SetFontStyle(font, style);
                                //std::cout << utf8.c_str();
                                letter_canvas.scale(kx, ky);
                                letter_canvas.clear(bgcolor);
                                letter_canvas.drawString(utf8.c_str(), 0, font_height - font_descent, *font, SkPaint(color));
                                
                                letter_image = letter_surface->makeImageSnapshot();

                                //std::cout << "backed: " << letter_image->isTextureBacked() << std::endl;

                                typesettingBox[litkey] = letter_image;
                            } else {
                                letter_image = typesettingBox[litkey];
                            }

                            canvas.drawImage(letter_image, rect.left(), rect.top());
                        }
                    //    matrix(row, col) = 0;
                    //}
                }
            }
            //texture = SDL_CreateTextureFromSurface(renderer, surface);
            //SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        }
        //SDL_RenderCopy(renderer, texture, NULL, &window_rect);
        // draw cursor
        VTermScreenCell cell;
        vterm_screen_get_cell(screen, cursor_pos, &cell);
		auto cur_color = SkColor4f::FromColor(SkColorSetRGB(cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue));

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

        if (ringing) {
            // TODO SDL_SetRenderDrawColor(renderer, 255,255,255,192 );
            // SDL_RenderFillRect(renderer, &window_rect);
			canvas.clear(color);
            ringing = 0;
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
			keyboard_key(VTERM_KEY_UP, (VTermModifier)mod);
			break;
		case XKB_KEY_Down:
			keyboard_key(VTERM_KEY_DOWN, (VTermModifier)mod);
			break;
		case XKB_KEY_Left:
			keyboard_key(VTERM_KEY_LEFT, (VTermModifier)mod);
			break;
		case XKB_KEY_Right:
			keyboard_key(VTERM_KEY_RIGHT, (VTermModifier)mod);
			break;
		case XKB_KEY_Page_Up:
			keyboard_key(VTERM_KEY_PAGEUP, (VTermModifier)mod);
			break;
		case XKB_KEY_Page_Down:
			keyboard_key(VTERM_KEY_PAGEDOWN, (VTermModifier)mod);
			break;
		case XKB_KEY_Home:
			keyboard_key(VTERM_KEY_HOME, (VTermModifier)mod);
			break;
		case XKB_KEY_End:
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
		
		//const int rows = 25, cols = 91;
        const int rows = 50, cols = 182;

		auto contents = std::make_shared<TermDisplayContents>(purist, rows, cols);
		auto contents_handler_for_skia = std::make_shared<pgs::DisplayContentsHandlerForSkia>(contents, enableOpenGL);
		
		purist->run(contents_handler_for_skia, contents);

		return 0;
	
	} catch (const purist::errcode_exception& ex) {
		fprintf(stderr, "%s\n", ex.what());
		return ex.errcode;
	}
}
