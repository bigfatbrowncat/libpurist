#pragma once

#include "Card.h"
#include "DumbBuffer.h"


class DumbBuffer;
class DumbBufferMapping;

class FrameBuffer {
private:
    bool added = false;
    const Card& card;
    Display& display;
public:
    //const std::shared_ptr<DumbBuffer> dumb;
    const std::shared_ptr<TargetSurface> target;
    const std::shared_ptr<DumbBufferMapping> mapping;

	const uint32_t framebuffer_id = 0;

    FrameBuffer(const Card& card, Display& display);
    void createAndAdd(int width, int height);
    void removeAndDestroy();
    virtual ~FrameBuffer();
};
