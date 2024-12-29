#pragma once

#include <cstdint>

namespace purist::graphics {

class TargetSurface {
public:
	virtual ~TargetSurface() = default;

	virtual uint32_t getWidth() const = 0;
	virtual uint32_t getHeight() const = 0;
	virtual uint32_t getStride() const = 0;
	virtual uint32_t getHandle() const = 0;

    virtual uint8_t* getMappedBuffer() const = 0;
};

}
