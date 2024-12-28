#pragma once

#include "Card.h"
#include "ModeConnector.h"

#include <purist/graphics/interfaces.h>

#include <memory>
#include <list>
#include <xf86drmMode.h>

namespace purist::graphics {

class DisplayImpl;

class Displays : protected std::list<std::shared_ptr<DisplayImpl>> {
    // Forbidding object copying
    Displays(const Displays& other) = delete;
    Displays& operator = (const Displays& other) = delete;

private:
    const Card& card;
    std::shared_ptr<DisplayContents> displayContents;
    bool opengl;
    Displays::iterator findDisplayOnConnector(const ModeConnector& conn);

public:
    Displays(const Card& card, bool opengl) : card(card), opengl(opengl) { }
    bool empty() const;
    void clear();
    void setDisplayContents(std::shared_ptr<DisplayContents> contents);
    void addNewlyConnectedToDrawingLoop();

    int updateHardwareConfiguration();

    std::shared_ptr<DisplayImpl> findDisplayConnectedToCrtc(uint32_t crtc_id) const;

    virtual ~Displays();
};

}