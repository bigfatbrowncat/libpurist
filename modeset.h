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

struct modeset_buf {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t stride = 0;
	uint32_t size = 0;
	uint32_t handle = 0;
	uint8_t *map = nullptr;
	uint32_t fb = 0;
};

struct modeset_dev {
	unsigned int front_buf = 0;
	struct modeset_buf bufs[2];

	drmModeModeInfo mode;
	uint32_t conn;
	uint32_t crtc;
	drmModeCrtc *saved_crtc;

	bool pflip_pending;
	bool cleanup;

	uint8_t r, g, b;
	bool r_up, g_up, b_up;
};

class errcode_exception : public std::runtime_error {
public:
    int errcode;
    std::string message;

    errcode_exception(int errcode, const std::string& message) : 
        errcode(errcode), message(message),
        std::runtime_error("System error " + std::to_string(errcode) + ": " + message) { }
};

class modeset {
private:
    struct page_flip_data {
        modeset* ms;
        modeset_dev* dev;
        page_flip_data(modeset* ms, modeset_dev* dev) : ms(ms), dev(dev) { }
    };

    static void modeset_page_flip_event(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);

    static std::set<std::shared_ptr<page_flip_data>> page_flip_data_cache;
    static std::list<std::shared_ptr<modeset_dev>> modeset_list;
public:
    int fd;

    int find_crtc(drmModeRes *res, drmModeConnector *conn, std::shared_ptr<modeset_dev> dev);
    int create_fb(struct modeset_buf *buf);
    void destroy_fb(struct modeset_buf *buf);
    int setup_dev(drmModeRes *res, drmModeConnector *conn, std::shared_ptr<modeset_dev> dev);
    int prepare();
    int set_modes();
    void draw();
    void draw_dev(modeset_dev* dev);
    void cleanup();
    
    modeset(const char *node);
    virtual ~modeset();
};
