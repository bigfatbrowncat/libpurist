#pragma once

#include "Card.h"
#include "DumbBufferMapping.h"

#include <cstdint>

class DumbBufferMapping;

class TargetSurface {
public:
	const Card& card;

	virtual uint32_t getWidth() const = 0;
	virtual uint32_t getHeight() const = 0;
	virtual uint32_t getStride() const = 0;
	virtual uint32_t getHandle() const = 0;

	TargetSurface(const Card& card) : card(card) { }
	virtual void makeCurrent() = 0;
	virtual void lock() = 0;
	virtual void swap() = 0;
	virtual void unlock() = 0;
    virtual void create(int width, int height) = 0;
    virtual void destroy() = 0;
	virtual ~TargetSurface() = default;
};

class DumbBuffer : public TargetSurface {
private:
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

	DumbBuffer(const Card& card);
	void makeCurrent() override { }
	void lock() override { }
	void swap() override { }
	void unlock() override { }
    void create(int width, int height) override;
    void destroy() override;
	virtual ~DumbBuffer();
};

#include <gbm.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

class GBMSurface : public TargetSurface {
private:
	bool created = false;

	gbm_surface* gbmSurface = nullptr;
	gbm_bo* gbmBO = nullptr;
	EGLSurface glSurface = EGL_NO_SURFACE;

	uint32_t width, height;

public:
	uint32_t getWidth() const override;
	uint32_t getHeight() const override;
	uint32_t getStride() const override;
	uint32_t getHandle() const override;

	GBMSurface(const Card& card);
    void create(int width, int height) override;
	void makeCurrent() override {
		eglMakeCurrent(card.glDisplay, glSurface, glSurface, card.glContext);
	}
    void destroy() override;
	void lock() override {
		gbmBO = gbm_surface_lock_front_buffer(gbmSurface);
	}

	virtual void swap() override {
		eglSwapBuffers(card.glDisplay, glSurface);	
	}

	void unlock() override {
		gbm_surface_release_buffer(gbmSurface, gbmBO);
	}

	virtual ~GBMSurface();
};