#pragma once

#include "Card.h"
#include "FrameBuffer.h"
#include "interfaces.h"

#include <xf86drmMode.h>

#include <array>

class Display {
    friend Card;

private:
    const Card& card;
    const Displays& displays;

	unsigned int current_framebuffer_index = 0;
	std::array<FrameBuffer, 2> framebuffers;

	std::shared_ptr<drmModeModeInfo> mode = nullptr;
	uint32_t crtc_id = 0;
	drmModeCrtc *saved_crtc = nullptr;


    std::shared_ptr<DisplayContents> contents = nullptr;

	int page_flips_pending = 0;

	bool destroying_in_progress = false;
    bool crtc_set_successfully = false;
    bool is_in_drawing_loop = false;

	uint32_t connector_id = 0;

public:

    Display(const Card& card, const Displays& displays, uint32_t connector_id, bool opengl)
            : card(card), displays(displays), 
              framebuffers { 
                FrameBuffer(card, *this, opengl), 
                FrameBuffer(card, *this, opengl) 
              }, 
              connector_id(connector_id) {}
    virtual ~Display();

    int connectDisplayToNotOccupiedCrtc(const drmModeRes *res, const drmModeConnector *conn);
    int setup(const drmModeRes *res, const drmModeConnector *conn);
    void draw();
    bool setCrtc(FrameBuffer *buf);
    bool isCrtcSet() const { return crtc_set_successfully; }
    bool isInDrawingLoop() const { return is_in_drawing_loop; }
    void updateInDrawingLoop(DisplayContentsFactory& factory);
    uint32_t getConnectorId() const { return connector_id; }
    //bool isPageFlipPending() const { return page_flip_pending; }
	bool isDestroyingInProgress() const { return destroying_in_progress; }

    void swap_buffers();

    static void modeset_page_flip_event(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);
};

