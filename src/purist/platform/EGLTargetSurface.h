#pragma once

#include "Card.h"

#include <purist/platform/interfaces.h>

#include <gbm.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

class EGLTargetSurface : public TargetSurface {
private:
	bool created = false;

	gbm_surface* gbmSurface = nullptr;
	gbm_bo* gbmBO = nullptr;
	EGLSurface glSurface = EGL_NO_SURFACE;

	const Card& card;
	uint32_t width, height;

public:
	uint32_t getWidth() const override;
	uint32_t getHeight() const override;
	uint32_t getStride() const override;
	uint32_t getHandle() const override;

	EGLTargetSurface(const Card& card);
    void create(int width, int height) override;
	void makeCurrent() override;
    void destroy() override;
	void lock() override;
	void swap() override;
	void unlock() override;

	uint8_t* getMappedBuffer() const override { return nullptr; }

	virtual ~EGLTargetSurface();
};
