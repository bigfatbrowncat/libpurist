#pragma once

#include "Card.h"
#include "FrameBuffer.h"
#include "ModeResources.h"
#include "ModeConnector.h"

#include "interfaces.h"

#include <xf86drmMode.h>
#include <array>

class Display {
    friend Card;

private:
    // Forbidding object copying
    Display(const Display& other) = delete;
    Display& operator = (const Display& other) = delete;

    const Card& card;
    const Displays& displays;

	unsigned int current_framebuffer_index = 0;
	std::array<std::unique_ptr<FrameBuffer>, 2> framebuffers;

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
                std::make_unique<FrameBuffer>(card, *this, opengl), 
                std::make_unique<FrameBuffer>(card, *this, opengl) 
              }, 
              connector_id(connector_id) {}
    virtual ~Display();

    int connectDisplayToNotOccupiedCrtc(const ModeResources& res, const ModeConnector& conn);
    int setup(const ModeResources& res, const ModeConnector& conn);
    void draw();
    bool setCrtc(FrameBuffer *buf);
    bool isCrtcSet() const { return crtc_set_successfully; }
    bool isInDrawingLoop() const { return is_in_drawing_loop; }
    void updateInDrawingLoop(DisplayContentsFactory& factory);
    uint32_t getConnectorId() const { return connector_id; }
	bool isDestroyingInProgress() const { return destroying_in_progress; }

    void swap_buffers();

    static void modeset_page_flip_event(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);
};

