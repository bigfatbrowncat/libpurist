#pragma once

#include "Card.h"

class DumbBufferTargetSurface;

class DumbBufferMapping {
private:
    // Forbidding object copying
    DumbBufferMapping(const DumbBufferMapping& other) = delete;
    DumbBufferMapping& operator = (const DumbBufferMapping& other) = delete;

    const Card& card;
    const DumbBufferTargetSurface& dumb;

public:
    uint8_t const* map = nullptr;

    DumbBufferMapping(const Card& card, const DumbBufferTargetSurface& dumb);
    void doMapping();
    void doUnmapping();
    virtual ~DumbBufferMapping();
};
