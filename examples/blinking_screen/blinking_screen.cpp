#include <purist/Platform.h>

#include <map>
#include <memory>
#include <cassert>

namespace pg = purist::graphics;
namespace pi = purist::input;

class ColoredScreenDisplayContents : public pg::DisplayContents, public pi::KeyboardHandler {
private:
	bool enableOpenGL;

public:
	uint8_t r, g, b;
	bool r_up, g_up, b_up;
	
	ColoredScreenDisplayContents(bool enableOpenGL) : enableOpenGL(enableOpenGL) { 
		r = rand() % 0xff;
		g = rand() % 0xff;
		b = rand() % 0xff;
		r_up = g_up = b_up = true;

	}

   /*
    * A short helper function to compute a changing color value. No need to
    * understand it.
    */

    static uint8_t next_color(bool *up, uint8_t cur, unsigned int mod)
    {
        uint8_t next;

        next = cur + (*up ? 1 : -1) * (rand() % mod);
        if ((*up && next < cur) || (!*up && next > cur)) {
            *up = !*up;
            next = cur;
        }

        return next;
    }

    std::list<std::shared_ptr<pg::Mode>>::const_iterator chooseMode(const std::list<std::shared_ptr<pg::Mode>>& modes) override {
		return modes.cbegin();
	}

    void drawIntoBuffer(std::shared_ptr<pg::Display> display, std::shared_ptr<pg::TargetSurface> target) override {
		
		int w = target->getWidth(), h = target->getHeight();

       	r = next_color(&r_up, r, 20);
        g = next_color(&g_up, g, 10);
        b = next_color(&b_up, b, 5);

		if (enableOpenGL) {
			assert(target->getMappedBuffer() == nullptr);

			glClearColor(1.0f/256*r, 1.0f/256*g, 1.0f/256*b, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			
		} else {
			assert(target->getMappedBuffer() != nullptr);
			int s = target->getStride();

			unsigned int j, k, off;
			uint32_t c = (r << 16) | (g << 8) | b;
			for (j = 0; j < h; ++j) {
				for (k = 0; k < w; ++k) {
					off = s * j + k * 4;
					*(uint32_t*)&(target->getMappedBuffer()[off]) = c;
				}
			}
		}

    }
	
	void onCharacter(pi::Keyboard& kbd, uint32_t utf8CharCode) override { }
    void onKeyPress(pi::Keyboard& kbd, uint32_t keyCode, pi::Modifiers mods, pi::Leds leds) override { }
    void onKeyRepeat(pi::Keyboard& kbd, uint32_t keyCode, pi::Modifiers mods, pi::Leds leds) override { }
    void onKeyRelease(pi::Keyboard& kbd, uint32_t keyCode, pi::Modifiers mods, pi::Leds leds) override { }
};

// class ColoredScreenDisplayApp : public pg::DisplayContents, public pi::KeyboardHandler {
// private:
// 	bool enableOpenGL;

// public:
// 	ColoredScreenDisplayApp(bool enableOpenGL) : enableOpenGL(enableOpenGL) { }

// 	std::shared_ptr<pg::DisplayContents> createDisplayContents(pg::Display& display) {
// 		auto contents = std::make_shared<ColoredScreenDisplayContents>(enableOpenGL);

// 		return contents;
// 	}



// };


int main(int argc, char **argv)
{
	try {
		bool enableOpenGL = true;

		purist::Platform purist(enableOpenGL);
		auto contentsFactory = std::make_shared<ColoredScreenDisplayContents>(enableOpenGL);
		
		purist.run(contentsFactory, contentsFactory);

		return 0;
	
	} catch (const purist::errcode_exception& ex) {
		fprintf(stderr, "%s\n", ex.what());
		return ex.errcode;
	}
}
