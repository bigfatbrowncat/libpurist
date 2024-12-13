#pragma once

#include "Card.h"
#include "FrameBuffer.h"
#include "ModeResources.h"
#include "ModeConnector.h"
#include "ModeCrtc.h"

#include <purist/platform/interfaces.h>

#include <xf86drmMode.h>
#include <array>

class Display {
    // Forbidding object copying
    Display(const Display& other) = delete;
    Display& operator = (const Display& other) = delete;

private:
    enum class State { 
        INITIALIZED = 0,
        CRTC_SET_SUCCESSFULLY = 1, 
        IN_DRAWING_LOOP = 2
    } state;

    const Card& card;
    const Displays& displays;

	unsigned int current_framebuffer_index = 0;
	std::array<std::unique_ptr<FrameBuffer>, 2> framebuffers;

	std::shared_ptr<drmModeModeInfo> mode = nullptr;
	
    uint32_t crtc_id = 0;
	std::unique_ptr<ModeCrtc> saved_crtc = nullptr;

    std::shared_ptr<DisplayContents> contents = nullptr;

	int page_flips_pending = 0;

    // bool crtc_set_successfully = false;
    // bool is_in_drawing_loop = false;
	bool destroying_in_progress = false;

	uint32_t connector_id = 0;

    void setCrtc(FrameBuffer *buf);
    void draw();
public:

    Display(const Card& card, const Displays& displays, uint32_t connector_id, bool opengl)
            : card(card), displays(displays), 
              framebuffers { 
                std::make_unique<FrameBuffer>(card, opengl), 
                std::make_unique<FrameBuffer>(card, opengl) 
              }, 
              connector_id(connector_id) {}
    virtual ~Display();

    int connectDisplayToNotOccupiedCrtc(const ModeResources& res, const ModeConnector& conn);
    int setup(const ModeResources& res, const ModeConnector& conn);
    void updateInDrawingLoop(DisplayContentsFactory& factory);
    uint32_t getConnectorId() const { return connector_id; }
    uint32_t getCrtcId() const { return crtc_id; }

    //void swap_buffers();

    static void modeset_page_flip_event(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);
};

