#pragma once

#include "Card.h"
#include "DumbBufferTargetSurfaceImpl.h"

#include "TargetSurfaceBackface.h"

class DumbBufferTargetSurfaceImpl;

class FrameBufferImpl : public FrameBuffer {
    // Forbidding object copying
    FrameBufferImpl(const FrameBufferImpl& other) = delete;
    FrameBufferImpl& operator = (const FrameBufferImpl& other) = delete;

private:
    bool added = false;
    const Card& card;

    static std::shared_ptr<TargetSurfaceBackface> target_for(bool opengl, const Card& card);
    const std::shared_ptr<TargetSurfaceBackface> target;
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
