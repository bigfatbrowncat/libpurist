#pragma once

#include "Card.h"
#include "DumbBufferTargetSurface.h"

#include <purist/platform/interfaces.h>

class DumbBufferTargetSurface;

class FrameBufferImpl : public FrameBuffer {
private:
    // Forbidding object copying
    FrameBufferImpl(const FrameBufferImpl& other) = delete;
    FrameBufferImpl& operator = (const FrameBufferImpl& other) = delete;

    bool added = false;
    const Card& card;

    static std::shared_ptr<TargetSurface> target_for(bool opengl, const Card& card);
    const std::shared_ptr<TargetSurface> target;
    const bool enableOpenGL;
    
public:
	const uint32_t framebuffer_id = 0;

    FrameBufferImpl(const Card& card, bool opengl);
    void createAndAdd(int width, int height);

    std::shared_ptr<TargetSurface> getTarget() const override { return target; }
    bool isOpenGLEnabled() const override { return enableOpenGL; }

    void removeAndDestroy();
    virtual ~FrameBufferImpl();
};
