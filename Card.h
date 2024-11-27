#pragma once

#include <xf86drmMode.h>

#include <memory>
#include <list>
#include <set>
#include <stdexcept>

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

struct VideoBuffer {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t stride = 0;
	uint32_t size = 0;
	uint32_t handle = 0;
	uint8_t *map = nullptr;
	uint32_t fb = 0;
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

    void update() {
       	r = DisplayContents::next_color(&r_up, r, 20);
        g = DisplayContents::next_color(&g_up, g, 10);
        b = DisplayContents::next_color(&b_up, b, 5);
    }
};

struct Display {
	unsigned int front_buf = 0;
	struct VideoBuffer bufs[2];

	drmModeModeInfo mode;
	uint32_t connector_id;
	uint32_t crtc;
	drmModeCrtc *saved_crtc;

	bool pflip_pending;
	bool cleanup;

    std::shared_ptr<DisplayContents> contents;
};


class errcode_exception : public std::runtime_error {
public:
    int errcode;
    std::string message;

    errcode_exception(int errcode, const std::string& message) : 
        errcode(errcode), message(message),
        std::runtime_error("System error " + std::to_string(errcode) + ": " + message) { }
};


class Card {
private:
    struct page_flip_data {
        Card* ms;
        Display* dev;
        page_flip_data(Card* ms, Display* dev) : ms(ms), dev(dev) { }
    };

    static void modeset_page_flip_event(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);

    static std::set<std::shared_ptr<page_flip_data>> page_flip_data_cache;
    static std::list<std::shared_ptr<Display>> displays_list;

    int find_crtc(drmModeRes *res, drmModeConnector *conn, std::shared_ptr<Display> dev);
    int create_fb(struct VideoBuffer *buf);
    void destroy_fb(struct VideoBuffer *buf);
    int setup_display(drmModeRes *res, drmModeConnector *conn, std::shared_ptr<Display> dev);
    void draw_dev(Display* dev);

    int fd;

public:
    Card(const char *node);

    int prepare();
    int set_modes();
    void draw();

    std::shared_ptr<DisplayContents> createDisplayContents();
    
    virtual ~Card();
};
