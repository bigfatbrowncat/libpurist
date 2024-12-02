#pragma once

#include "Card.h"

class DumbBuffer;
class DumbBufferMapping;

class FrameBuffer {
private:
    bool added = false;
    const Card& card;

public:
    const std::shared_ptr<DumbBuffer> dumb;
    const std::shared_ptr<DumbBufferMapping> mapping;

	const uint32_t framebuffer_id = 0;

    FrameBuffer(const Card& card);
    void createAndAdd(int width, int height);
    void removeAndDestroy();
    virtual ~FrameBuffer();
};
