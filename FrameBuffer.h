#pragma once

#include "Card.h"
#include "DumbBufferTargetSurface.h"

class DumbBufferTargetSurface;

class FrameBuffer {
private:
    // Forbidding object copying
    FrameBuffer(const FrameBuffer& other) = delete;
    FrameBuffer& operator = (const FrameBuffer& other) = delete;

    bool added = false;
    const Card& card;

    static std::shared_ptr<TargetSurface> target_for(bool opengl, const Card& card);
public:
    const std::shared_ptr<TargetSurface> target;
	const uint32_t framebuffer_id = 0;
    const bool enableOpenGL;

    FrameBuffer(const Card& card, bool opengl);
    void createAndAdd(int width, int height);
    void removeAndDestroy();
    virtual ~FrameBuffer();
};
