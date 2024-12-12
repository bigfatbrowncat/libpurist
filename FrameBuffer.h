#pragma once

#include "Card.h"
#include "DumbBuffer.h"

class DumbBuffer;

class FrameBuffer {
private:
    // Forbidding object copying
    FrameBuffer(const FrameBuffer& other) = delete;
    FrameBuffer& operator = (const FrameBuffer& other) = delete;

    bool added = false;
    const Card& card;
    Display& display;

    static std::shared_ptr<TargetSurface> target_for(bool opengl, const Card& card);
public:
    const std::shared_ptr<TargetSurface> target;
	const uint32_t framebuffer_id = 0;
    const bool enableOpenGL;

    FrameBuffer(const Card& card, Display& display, bool opengl);
    void createAndAdd(int width, int height);
    void removeAndDestroy();
    virtual ~FrameBuffer();
};
