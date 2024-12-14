#include <purist/platform/Platform.h>

class ColoredScreenDisplayContents : public purist::platform::DisplayContents {
public:
	uint8_t r, g, b;
	bool r_up, g_up, b_up;

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

    void drawIntoBuffer(purist::platform::TargetSurface& target) override {
       	r = next_color(&r_up, r, 20);
        g = next_color(&g_up, g, 10);
        b = next_color(&b_up, b, 5);

		if (!target.getMappedBuffer()) {
			
			glClearColor(1.0f/256*r, 1.0f/256*g, 1.0f/256*b, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

		} else {

			unsigned int j, k, off;

			uint32_t h = target.getHeight();
			uint32_t w = target.getWidth(); 
			uint32_t s = target.getStride(); 
			uint32_t c = (r << 16) | (g << 8) | b;
			for (j = 0; j < h; ++j) {
				for (k = 0; k < w; ++k) {
					off = s * j + k * 4;
					*(uint32_t*)&(target.getMappedBuffer()[off]) = c;
				}
			}
		}
    }
};

class ColoredScreenDisplayContentsFactory : public purist::platform::DisplayContentsFactory {
public:
	std::shared_ptr<purist::platform::DisplayContents> createDisplayContents(purist::platform::Display& display) {
		auto contents = std::make_shared<ColoredScreenDisplayContents>();
		contents->r = rand() % 0xff;
		contents->g = rand() % 0xff;
		contents->b = rand() % 0xff;
		contents->r_up = contents->g_up = contents->b_up = true;
		return contents;
	}

};


int main(int argc, char **argv)
{
	try {
		bool enableOpenGL = true;

		purist::platform::Platform purist(enableOpenGL);
		auto contentsFactory = std::make_shared<ColoredScreenDisplayContentsFactory>();
		
		purist.run(contentsFactory);

		return 0;
	
	} catch (const purist::platform::errcode_exception& ex) {
		fprintf(stderr, "%s\n", ex.what());
		return ex.errcode;
	}
}
