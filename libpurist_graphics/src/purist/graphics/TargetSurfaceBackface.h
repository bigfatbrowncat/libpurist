#pragma once

#include <purist/graphics/TargetSurface.h>

namespace purist::graphics {

class TargetSurfaceBackface : public TargetSurface {
public:
	virtual ~TargetSurfaceBackface() = default;

	virtual void makeCurrent() = 0;
	virtual void lock() = 0;
	virtual void swap() = 0;
	virtual void unlock() = 0;
    virtual void create(int width, int height) = 0;
    virtual void destroy() = 0;
};

}