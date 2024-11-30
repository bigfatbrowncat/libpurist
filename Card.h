#pragma once

#include <xf86drmMode.h>

#include <memory>
#include <list>
#include <set>
#include <stdexcept>


class errcode_exception : public std::runtime_error {
public:
    int errcode;
    std::string message;

    errcode_exception(int errcode, const std::string& message) : 
        std::runtime_error("System error " + std::to_string(errcode) + ": " + message), 
        errcode(errcode), 
        message(message) { }
};


class Card;
class Display;
class Displays;
class DumbBufferMapping;
class FrameBuffer;

class DisplayContents {
public:
    virtual ~DisplayContents() = default;

    virtual void drawIntoBuffer(FrameBuffer* buf) = 0;
};

class DisplayContentsFactory {
public:
    virtual ~DisplayContentsFactory() = default;

    virtual std::shared_ptr<DisplayContents> createDisplayContents() = 0;
};

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
    uint8_t const* map = nullptr;

    const Card& card;
    const FrameBuffer& buf;
    const DumbBuffer& dumb;

    DumbBufferMapping(const Card& card, const FrameBuffer& buf, const DumbBuffer& dumb);
    void doMapping();
    void doUnmapping();
    virtual ~DumbBufferMapping();
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

    void runDrawingLoop();
    
    virtual ~Card();

    std::shared_ptr<Displays> displays;
};

class Display {
private:
    /*static*/ std::set<std::shared_ptr<Card::page_flip_callback_data>> page_flip_callback_data_cache;

public:
    const Card& card;
    const Displays& displays;

	unsigned int front_buf = 0;
	FrameBuffer bufs[2];

	std::shared_ptr<drmModeModeInfo> mode = nullptr;
	uint32_t connector_id = 0;
	uint32_t crtc_id = 0;
	drmModeCrtc *saved_crtc = nullptr;

	bool pflip_pending = false;
	bool cleanup = false;

    std::shared_ptr<DisplayContents> contents = nullptr;

    bool crtc_set_successfully = false;
    bool is_in_drawing_loop = false;

    Display(const Card& card, const Displays& displays)
            : card(card), displays(displays), bufs { FrameBuffer(card), FrameBuffer(card) } {}
    virtual ~Display();

    int connectDisplayToNotOccupiedCrtc(drmModeRes *res, drmModeConnector *conn);

    int setup(drmModeRes *res, drmModeConnector *conn);
    void draw();
    bool setCrtc();
};


class Displays : protected std::list<std::shared_ptr<Display>> {
    friend class Display;
private:
    const Card& card;
    std::shared_ptr<DisplayContentsFactory> displayContentsFactory;
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