#pragma once

#include "Card.h"

#include "interfaces.h"

#include <memory>
#include <list>
#include <xf86drmMode.h>

class Displays : protected std::list<std::shared_ptr<Display>> {
    friend class Card;
    friend class Display;
private:
    const Card& card;
    std::shared_ptr<DisplayContentsFactory> displayContentsFactory;
    bool opengl;
    Displays::iterator findDisplayOnConnector(const drmModeConnector *conn);

public:
    bool empty() const {
        return std::list<std::shared_ptr<Display>>::empty();
    }

    Displays(const Card& card, bool opengl) : card(card), opengl(opengl) { }
    void setDisplayContentsFactory(std::shared_ptr<DisplayContentsFactory> factory);
    void addNewlyConnectedToDrawingLoop();

    int updateHardwareConfiguration();

    virtual ~Displays();
};

