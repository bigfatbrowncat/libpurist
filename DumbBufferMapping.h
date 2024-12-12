#pragma once

#include "Card.h"
#include "DumbBuffer.h"

class DumbBuffer;

class DumbBufferMapping {
private:
    // Forbidding object copying
    DumbBufferMapping(const DumbBufferMapping& other) = delete;
    DumbBufferMapping& operator = (const DumbBufferMapping& other) = delete;

    const Card& card;
    const DumbBuffer& dumb;

public:
    uint8_t const* map = nullptr;

    DumbBufferMapping(const Card& card, const DumbBuffer& dumb);
    void doMapping();
    void doUnmapping();
    virtual ~DumbBufferMapping();
};
