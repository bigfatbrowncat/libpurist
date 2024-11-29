#include "Card.h"

#include <cstdio>
#include <memory>

class ColoredScreenDisplayContents : public DisplayContents {
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

    void drawIntoBuffer(FrameBuffer* buf) override {
       	r = next_color(&r_up, r, 20);
        g = next_color(&g_up, g, 10);
        b = next_color(&b_up, b, 5);

    	unsigned int j, k, off;

       	for (j = 0; j < buf->dumb->height; ++j) {
            for (k = 0; k < buf->dumb->width; ++k) {
                off = buf->dumb->stride * j + k * 4;
                *(uint32_t*)&buf->mapping->map[off] =
                        (r << 16) | (g << 8) | b;
            }
        }
    }
};

class ColoredScreenDisplayContentsFactory : public DisplayContentsFactory {
public:
	std::shared_ptr<DisplayContents> createDisplayContents() {
		auto contents = std::make_shared<ColoredScreenDisplayContents>();
		contents->r = rand() % 0xff;
		contents->g = rand() % 0xff;
		contents->b = rand() % 0xff;
		contents->r_up = contents->g_up = contents->b_up = true;
		return contents;
	}

};

/*
 * main() also stays the same.
 */

int main(int argc, char **argv)
{
	try {
		const char *card;

		/* check which DRM device to open */
		if (argc > 1)
			card = argv[1];
		else
			card = "/dev/dri/card1";

		fprintf(stderr, "using card '%s'\n", card);

		auto ms = std::make_unique<Card>(card);
		
		ms->displays->setDisplayContentsFactory(std::make_shared<ColoredScreenDisplayContentsFactory>());

		/* draw some colors for 5seconds */
		ms->runDrawingLoop();

		ms = nullptr;
		fprintf(stderr, "exiting\n");
		return 0;
	
	} catch (const errcode_exception& ex) {
		fprintf(stderr, "%s\n", ex.what());
		return ex.errcode;
	}
}