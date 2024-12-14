#pragma once

#include <memory>
#include <cstdint>

namespace purist::platform {

// Implemented on the library side

class TargetSurface {
public:
	virtual ~TargetSurface() = default;

	virtual uint32_t getWidth() const = 0;
	virtual uint32_t getHeight() const = 0;
	virtual uint32_t getStride() const = 0;
	virtual uint32_t getHandle() const = 0;

    virtual uint8_t* getMappedBuffer() const = 0;
};

class FrameBuffer {
public:
    virtual ~FrameBuffer() = default;

    virtual std::shared_ptr<TargetSurface> getTarget() const = 0;
    virtual bool isOpenGLEnabled() const = 0;
};

class Display {
public:
    virtual ~Display() = default;

    virtual uint32_t getConnectorId() const = 0;
};


// Implemented on the user side

class DisplayContents {
public:
    virtual ~DisplayContents() = default;

    virtual void drawIntoBuffer(TargetSurface& target) = 0;
};

class DisplayContentsFactory {
public:
    virtual ~DisplayContentsFactory() = default;

    virtual std::shared_ptr<DisplayContents> createDisplayContents(Display& display) = 0;
};

}