#pragma once

#include "Card.h"
#include "DumbBuffer.h"

class DumbBufferMapping {
private:
    const Card& card;
    const DumbBuffer& dumb;

public:
    uint8_t const* map = nullptr;

    DumbBufferMapping(const Card& card, const DumbBuffer& dumb);
    void doMapping();
    void doUnmapping();
    virtual ~DumbBufferMapping();
};
