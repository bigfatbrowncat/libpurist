#pragma once

#include "Card.h"

namespace purist::graphics {

class DumbBufferTargetSurfaceImpl;

class DumbBufferMapping {
private:
    // Forbidding object copying
    DumbBufferMapping(const DumbBufferMapping& other) = delete;
    DumbBufferMapping& operator = (const DumbBufferMapping& other) = delete;

    const Card& card;
    const DumbBufferTargetSurfaceImpl& dumb;

public:
    uint8_t const* map = nullptr;

    DumbBufferMapping(const Card& card, const DumbBufferTargetSurfaceImpl& dumb);
    void doMapping();
    void doUnmapping();
    virtual ~DumbBufferMapping();
};

}