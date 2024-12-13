#pragma once

#include <memory>
#include <cstdint>

class TargetSurface {
public:
	virtual ~TargetSurface() = default;

	virtual uint32_t getWidth() const = 0;
	virtual uint32_t getHeight() const = 0;
	virtual uint32_t getStride() const = 0;
	virtual uint32_t getHandle() const = 0;

	virtual void makeCurrent() = 0;
	virtual void lock() = 0;
	virtual void swap() = 0;
	virtual void unlock() = 0;
    virtual void create(int width, int height) = 0;
    virtual void destroy() = 0;

    virtual uint32_t* getMappedBuffer() const = 0;
};

class FrameBufferInterface {
public:
    virtual ~FrameBufferInterface() = default;

    virtual std::shared_ptr<TargetSurface> getTarget() const = 0;
    virtual bool isOpenGLEnabled() const = 0;
};

class DisplayContents {
public:
    virtual ~DisplayContents() = default;

    virtual void drawIntoBuffer(FrameBufferInterface* buf) = 0;
};

class DisplayContentsFactory {
public:
    virtual ~DisplayContentsFactory() = default;

    virtual std::shared_ptr<DisplayContents> createDisplayContents() = 0;
};

