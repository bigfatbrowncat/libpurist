#pragma once

#include <xf86drmMode.h>

#include <memory>
#include <list>


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

class modeset {
public:
    static int modeset_find_crtc(int fd, drmModeRes *res, drmModeConnector *conn, std::shared_ptr<modeset_dev> dev);
    static int modeset_create_fb(int fd, struct modeset_buf *buf);
    static void modeset_destroy_fb(int fd, struct modeset_buf *buf);
    static int modeset_setup_dev(int fd, drmModeRes *res, drmModeConnector *conn, std::shared_ptr<modeset_dev> dev);
    static int modeset_open(int *out, const char *node);
    static int modeset_prepare(int fd);
    static void modeset_draw(int fd);
    static void modeset_draw_dev(int fd, modeset_dev* dev);
    static void modeset_cleanup(int fd);
    
    static void modeset_page_flip_event(int fd, unsigned int frame,
				    unsigned int sec, unsigned int usec,
				    void *data);
};
