#pragma once

#include <cstdint>

namespace purist::graphics {

class Mode {
public:
	virtual ~Mode() = default;

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual float getFreq() const = 0;
};

}