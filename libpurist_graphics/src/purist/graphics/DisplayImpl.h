#pragma once

#include "Card.h"
#include "FrameBufferImpl.h"
#include "ModeResources.h"
#include "ModeConnector.h"
#include "ModeModeInfo.h"
#include "ModeCrtc.h"

#include <purist/graphics/Display.h>

#include <xf86drmMode.h>
#include <array>
#include <memory>

namespace purist::graphics {

class DisplayImpl : public Display, public std::enable_shared_from_this<DisplayImpl> {
    // Forbidding object copying
    DisplayImpl(const DisplayImpl& other) = delete;
    DisplayImpl& operator = (const DisplayImpl& other) = delete;

private:
    enum class State { 
        INITIALIZED = 0,
        CRTC_SET_SUCCESSFULLY = 1, 
        IN_DRAWING_LOOP = 2
    } state = State::INITIALIZED;

    const Card& card;
    const Displays& displays;

	unsigned int current_framebuffer_index = 0;
	std::array<std::unique_ptr<FrameBufferImpl>, 2> framebuffers;

	std::shared_ptr<ModeModeInfo> mode = nullptr;
	
    uint32_t crtc_id = 0;
	std::unique_ptr<ModeCrtc> saved_crtc = nullptr;

    std::shared_ptr<DisplayContentsHandler> contents = nullptr;

	int page_flips_pending = 0;

    // bool crtc_set_successfully = false;
    // bool is_in_drawing_loop = false;
	bool destroying_in_progress = false;

	uint32_t connector_id = 0;

    void setCrtc(FrameBufferImpl *buf);
    void draw();

public:
    DisplayImpl(const Card& card, const Displays& displays, uint32_t connector_id, bool opengl)
            : card(card), displays(displays), 
              framebuffers { 
                std::make_unique<FrameBufferImpl>(card, opengl), 
                std::make_unique<FrameBufferImpl>(card, opengl) 
              }, 
              connector_id(connector_id) {}
    virtual ~DisplayImpl();

    int connectDisplayToNotOccupiedCrtc(const ModeResources& res, const ModeConnector& conn);
    int setup(const ModeResources& res, const ModeConnector& conn);
    void setContentsHandler(std::shared_ptr<DisplayContentsHandler> contents);
    void updateInDrawingLoop();
    uint32_t getConnectorId() const override { return connector_id; }
    uint32_t getFramebuffersCount() const override { return framebuffers.size(); }
    uint32_t getCrtcId() const { return crtc_id; }
    const Mode& getMode() const override { return *mode; }


    static void modeset_page_flip_event(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);
};

}