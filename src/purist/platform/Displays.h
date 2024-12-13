#pragma once

#include "Card.h"
#include "ModeConnector.h"

#include <purist/platform/interfaces.h>

#include <memory>
#include <list>
#include <xf86drmMode.h>

class Display;

class Displays : protected std::list<std::shared_ptr<Display>> {
    // Forbidding object copying
    Displays(const Displays& other) = delete;
    Displays& operator = (const Displays& other) = delete;

private:
    const Card& card;
    std::shared_ptr<DisplayContentsFactory> displayContentsFactory;
    bool opengl;
    Displays::iterator findDisplayOnConnector(const ModeConnector& conn);

public:
    Displays(const Card& card, bool opengl) : card(card), opengl(opengl) { }
    bool empty() const;
    void clear();
    void setDisplayContentsFactory(std::shared_ptr<DisplayContentsFactory> factory);
    void addNewlyConnectedToDrawingLoop();

    int updateHardwareConfiguration();

    std::shared_ptr<Display> findDisplayConnectedToCrtc(uint32_t crtc_id) const;

    virtual ~Displays();
};

