#include <purist/Platform.h>
#include <purist/graphics/TargetSurface.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>

#include <map>
#include <memory>
#include <cassert>

namespace p = purist;
namespace pg = purist::graphics;
namespace pi = purist::input;

class ColoredScreenDisplayContents : public pg::DisplayContentsHandler, public pi::KeyboardHandler {
private:
	std::weak_ptr<p::Platform> platform;
	bool enableOpenGL;

public:
	uint8_t r, g, b;
	bool r_up, g_up, b_up;
	
	ColoredScreenDisplayContents(std::weak_ptr<p::Platform> platform, bool enableOpenGL) : platform(platform), enableOpenGL(enableOpenGL) { 
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
		assert(target->getWidth() > 0 && target->getHeight() > 0);
		unsigned int w = target->getWidth(), h = target->getHeight();

       	r = next_color(&r_up, r, 20);
        g = next_color(&g_up, g, 10);
        b = next_color(&b_up, b, 5);

		if (enableOpenGL) {
			assert(target->getMappedBuffer() == nullptr);

			glClearColor(1.0f / 256 * r, 
			             1.0f / 256 * g, 
						 1.0f / 256 * b, 1.0f);
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
	
	void onCharacter(pi::Keyboard& kbd, char32_t charCode, pi::Modifiers mods, pi::Leds leds) override { 
		platform.lock()->stop();
	}

    void onKeyPress(pi::Keyboard& kbd, uint32_t keyCode, pi::Modifiers mods, pi::Leds leds, bool repeat) override { }
    void onKeyRelease(pi::Keyboard& kbd, uint32_t keyCode, pi::Modifiers mods, pi::Leds leds) override { }
};


int main(int argc, char **argv)
{
	try {
		bool enableOpenGL = true;

		std::shared_ptr<p::Platform> purist = std::make_shared<p::Platform>(enableOpenGL);
		auto contentsGenerator = std::make_shared<ColoredScreenDisplayContents>(purist, enableOpenGL);
		
		purist->run(contentsGenerator, contentsGenerator);

		return 0;
	
	} catch (const p::errcode_exception& ex) {
		fprintf(stderr, "%s\n", ex.what());
		return ex.errcode;
	}
}
