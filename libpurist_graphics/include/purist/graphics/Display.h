#pragma once

#include <cstdint>

namespace purist::graphics {

class Mode;

class Display {
public:
    virtual ~Display() = default;
    virtual const Mode& getMode() const = 0;
    virtual uint32_t getConnectorId() const = 0;
};

}
