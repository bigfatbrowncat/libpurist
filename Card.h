#pragma once

#include <xf86drmMode.h>

#include <memory>
#include <list>
#include <set>
#include <stdexcept>

class Card;

/*
 * modeset_buf and modeset_dev stay mostly the same. But 6 new fields are added
 * to modeset_dev: r, g, b, r_up, g_up, b_up. They are used to compute the
 * current color that is drawn on this output device. You can ignore them as
 * they aren't important for this example.
 * The modeset-double-buffered.c example used exactly the same fields but as
 * local variables in modeset_draw().
 *
 * The \pflip_pending variable is true when a page-flip is currently pending,
 * that is, the kernel will flip buffers on the next vertical blank. The
 * \cleanup variable is true if the device is currently cleaned up and no more
 * pageflips should be scheduled. They are used to synchronize the cleanup
 * routines.
 */

class DumbBuffer {
private:
    bool created = false;

public:
	const Card& card;

	const uint32_t stride;
	const uint32_t size;
	const uint32_t handle;

    const int width, height;

	DumbBuffer(const Card& card);
    void create(int width, int height);
    void destroy();
	virtual ~DumbBuffer();
};


class DumbBufferMapping;

class FrameBuffer {
private:
    bool added = false;

public:
    const Card& card;

    const std::shared_ptr<DumbBuffer> dumb;
    const std::shared_ptr<DumbBufferMapping> mapping;

	const uint32_t framebuffer_id = 0;

    FrameBuffer(const Card& card);
    void createAndAdd(int width, int height);
    void removeAndDestroy();
    virtual ~FrameBuffer();
};


class DumbBufferMapping {
public:
    uint8_t const* map;

    const Card& card;
    const FrameBuffer& buf;
    const DumbBuffer& dumb;

    DumbBufferMapping(const Card& card, const FrameBuffer& buf, const DumbBuffer& dumb);
    void doMapping();
    void doUnmapping();
    virtual ~DumbBufferMapping();
};


struct DisplayContents {
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

    void drawIntoBuffer(FrameBuffer* buf) {
       	r = DisplayContents::next_color(&r_up, r, 20);
        g = DisplayContents::next_color(&g_up, g, 10);
        b = DisplayContents::next_color(&b_up, b, 5);

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

class Display;
class Displays;


class errcode_exception : public std::runtime_error {
public:
    int errcode;
    std::string message;

    errcode_exception(int errcode, const std::string& message) : 
        std::runtime_error("System error " + std::to_string(errcode) + ": " + message), 
        errcode(errcode), 
        message(message) { }
};


class Card {
    friend class Displays;
    friend class Display;
private:
    struct page_flip_callback_data {
        const Card* ms;
        Display* dev;
        page_flip_callback_data(const Card* ms, Display* dev) : ms(ms), dev(dev) { }
    };

    static void modeset_page_flip_event(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);

public:
    const int fd;

    Card(const char *node);

    int prepare();
    bool setAllDisplaysModes();
    void runDrawingLoop();

    
    virtual ~Card();

    std::shared_ptr<Displays> displays;
};

class Display {
    static std::set<std::shared_ptr<Card::page_flip_callback_data>> page_flip_callback_data_cache;
public:
    const Card& card;
    const Displays& displays;

	unsigned int front_buf = 0;
	FrameBuffer bufs[2];

	drmModeModeInfo mode;
	uint32_t connector_id;
	uint32_t crtc_id;
	drmModeCrtc *saved_crtc;

	bool pflip_pending;
	bool cleanup;

    std::shared_ptr<DisplayContents> contents;

    bool mode_set_successfully = false;

    Display(const Card& card, const Displays& displays) : card(card), displays(displays), bufs { FrameBuffer(card), FrameBuffer(card) } {}
    void draw();
    int setup(drmModeRes *res, drmModeConnector *conn);

};


class Displays : protected std::list<std::shared_ptr<Display>> {
private:
    const Card& card;
    void drawOneDisplayContents(Display* dev);

public:
    bool empty() const {
        return std::list<std::shared_ptr<Display>>::empty();
    }

    void add(std::shared_ptr<Display> display) {
        this->push_front(display);
    }

    Displays(const Card& card) : card(card) { }
    std::shared_ptr<DisplayContents> createDisplayContents();
    bool setAllDisplaysModes();
    void draw() {
       	/* redraw all outputs */
        for (auto& iter : *this) {
            if (iter->mode_set_successfully) {
                iter->contents = createDisplayContents();
                iter->draw();
            }
        }

    }
    int find_crtc(drmModeRes *res, drmModeConnector *conn, Display& display) const;
    virtual ~Displays();
};