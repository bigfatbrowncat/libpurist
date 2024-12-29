#pragma once

#include <memory>

namespace purist::graphics {

class TargetSurface;

class FrameBuffer {
public:
    virtual ~FrameBuffer() = default;

    virtual std::shared_ptr<TargetSurface> getTarget() const = 0;
    virtual bool isOpenGLEnabled() const = 0;
};

}
