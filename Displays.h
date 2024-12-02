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

