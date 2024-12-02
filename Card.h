#pragma once

#include "exceptions.h"
#include "interfaces.h"

#include <xf86drmMode.h>

#include <memory>
#include <list>
#include <set>


class Card;
class Display;
class Displays;
class DumbBufferMapping;


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


class FrameBuffer {
private:
    bool added = false;
    const Card& card;

public:
    const std::shared_ptr<DumbBuffer> dumb;
    const std::shared_ptr<DumbBufferMapping> mapping;

	const uint32_t framebuffer_id = 0;

    FrameBuffer(const Card& card);
    void createAndAdd(int width, int height);
    void removeAndDestroy();
    virtual ~FrameBuffer();
};


class DumbBufferMapping {
private:
    const Card& card;
    const DumbBuffer& dumb;

public:
    uint8_t const* map = nullptr;

    DumbBufferMapping(const Card& card, const DumbBuffer& dumb);
    void doMapping();
    void doUnmapping();
    virtual ~DumbBufferMapping();
};


class Card {
    friend class Displays;
    friend class Display;

public:
    const int fd;
    const std::shared_ptr<Displays> displays;

    Card(const char *node);
    virtual ~Card();

    void runDrawingLoop();
};

class Display {
    friend Card;

private:
    struct page_flip_callback_data {
        const Card* ms;
        Display* dev;
        page_flip_callback_data(const Card* ms, Display* dev) : ms(ms), dev(dev) { }
    };

    static std::set<std::shared_ptr<page_flip_callback_data>> page_flip_callback_data_cache;

    const Card& card;
    const Displays& displays;

	unsigned int front_buf = 0;
	FrameBuffer bufs[2];

	std::shared_ptr<drmModeModeInfo> mode = nullptr;
	uint32_t crtc_id = 0;
	drmModeCrtc *saved_crtc = nullptr;

	bool page_flip_pending = false;
	bool destroying_in_progress = false;

    std::shared_ptr<DisplayContents> contents = nullptr;

    bool crtc_set_successfully = false;
    bool is_in_drawing_loop = false;

	uint32_t connector_id = 0;

public:

    Display(const Card& card, const Displays& displays, uint32_t connector_id)
            : card(card), displays(displays), bufs { FrameBuffer(card), FrameBuffer(card) }, connector_id(connector_id) {}
    virtual ~Display();

    int connectDisplayToNotOccupiedCrtc(const drmModeRes *res, const drmModeConnector *conn);
    int setup(const drmModeRes *res, const drmModeConnector *conn);
    void draw();
    bool setCrtc();
    bool isCrtcSet() const { return crtc_set_successfully; }
    bool isInDrawingLoop() const { return is_in_drawing_loop; }
    void updateInDrawingLoop(DisplayContentsFactory& factory);
    uint32_t getConnectorId() const { return connector_id; }
    bool isPageFlipPending() const { return page_flip_pending; }
	bool isDestroyingInProgress() const { return destroying_in_progress; }

    static void modeset_page_flip_event(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);
};


class Displays : protected std::list<std::shared_ptr<Display>> {
    friend class Card;
    friend class Display;
private:
    const Card& card;
    std::shared_ptr<DisplayContentsFactory> displayContentsFactory;
    std::shared_ptr<Display> findDisplayOnConnector(const drmModeConnector *conn) const;

public:
    bool empty() const {
        return std::list<std::shared_ptr<Display>>::empty();
    }

    Displays(const Card& card) : card(card) { }
    void setDisplayContentsFactory(std::shared_ptr<DisplayContentsFactory> factory);
    void updateDisplaysInDrawingLoop();

    int update();
    bool setAllCrtcs();

    virtual ~Displays();
};

class ModeResources {
public:
	const drmModeRes *resources;

	ModeResources(const Card& card);
	virtual ~ModeResources();
};

class ModeConnector {
public:
    const drmModeConnector *connector;

	ModeConnector(const Card& card, uint32_t connector_id);
    ModeConnector(const Card& card, const ModeResources& resources, size_t index);

	virtual ~ModeConnector();

};