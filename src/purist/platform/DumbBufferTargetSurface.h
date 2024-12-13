#pragma once

#include "Card.h"
#include "DumbBufferMapping.h"

#include "TargetSurfaceBackface.h"

#include <cstdint>

class DumbBufferMapping;

class DumbBufferTargetSurface : public TargetSurfaceBackface {
private:
	const Card& card;
    bool created = false;

	uint32_t stride;
	uint32_t size;
	uint32_t handle;
	uint32_t width, height;

public:
	uint32_t getWidth() const override { return width; }
	uint32_t getHeight() const override { return height; }
	uint32_t getStride() const override { return stride; }
	uint32_t getHandle() const override { return handle; }

	uint32_t getSize() const { return size; }

    const std::shared_ptr<DumbBufferMapping> mapping;

	DumbBufferTargetSurface(const Card& card);
	void makeCurrent() override { }
	void lock() override { }
	void swap() override { }
	void unlock() override { }
    void create(int width, int height) override;
    void destroy() override;

	uint8_t* getMappedBuffer() const override { return (uint8_t*)mapping->map; }

	virtual ~DumbBufferTargetSurface();
};
