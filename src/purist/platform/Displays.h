#pragma once

#include "Card.h"
#include "ModeConnector.h"

#include <purist/platform/interfaces.h>

#include <memory>
#include <list>
#include <xf86drmMode.h>

class Display;

class Displays : protected std::list<std::shared_ptr<Display>> {
    //friend class Card;
    //friend class Display;
private:
    // Forbidding object copying
    Displays(const Displays& other) = delete;
    Displays& operator = (const Displays& other) = delete;

    const Card& card;
    std::shared_ptr<DisplayContentsFactory> displayContentsFactory;
    bool opengl;
    Displays::iterator findDisplayOnConnector(const ModeConnector& conn);

public:
    bool empty() const {
        return std::list<std::shared_ptr<Display>>::empty();
    }

    void clear() { std::list<std::shared_ptr<Display>>::clear(); }

    Displays(const Card& card, bool opengl) : card(card), opengl(opengl) { }
    void setDisplayContentsFactory(std::shared_ptr<DisplayContentsFactory> factory);
    void addNewlyConnectedToDrawingLoop();

    int updateHardwareConfiguration();

    std::shared_ptr<Display> findDisplayConnectedToCrtc(uint32_t crtc_id) const;

    virtual ~Displays();
};

