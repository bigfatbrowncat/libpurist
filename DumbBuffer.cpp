#include "DumbBuffer.h"
#include "exceptions.h"

#include <EGL/egl.h>
#include <gbm.h>
#include <xf86drm.h>

#include <cassert>
#include <cstring>
#include <cerrno>
#include <string>

DumbBuffer::DumbBuffer(const Card& card) 
	: TargetSurface(card), stride(0), size(0), handle(0), width(0), height(0) {
}

void DumbBuffer::create(int width, int height) {
	struct drm_mode_create_dumb creq;
	/* create dumb buffer */
	memset(&creq, 0, sizeof(creq));
	creq.width = width;
	creq.height = height;
	creq.bpp = 32;
	int ret = drmIoctl(card.fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
	if (ret < 0) {
		throw errcode_exception(-errno, std::string("cannot create dumb buffer. ") + strerror(errno));
	}

	this->stride = creq.pitch;
	this->size = creq.size;
	this->handle = creq.handle;

	this->width = width;
	this->height = height;

	created = true;
}

void DumbBuffer::destroy() {
	assert(created);

	struct drm_mode_destroy_dumb dreq;
	memset(&dreq, 0, sizeof(dreq));
	dreq.handle = handle;

	int ret = drmIoctl(card.fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
	if (ret < 0) {
		throw errcode_exception(-errno, std::string("cannot destroy dumb buffer. ") + strerror(errno));
	}
	created = false;
}

DumbBuffer::~DumbBuffer() {
	if (created) {
		destroy();
	}
}


GBMSurface::GBMSurface(const Card& card) 
	: TargetSurface(card), width(0), height(0) {
}

void GBMSurface::create(int width, int height) {
	this->width = width;
	this->height = height;

	gbmSurface = gbm_surface_create(card.getGBMDevice(),
			width, height,
			GBM_FORMAT_XRGB8888,
			GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!gbmSurface) {
		throw errcode_exception(-errno, std::string("failed to create the GBM surface. ") + strerror(errno));
	}

	glSurface = eglCreateWindowSurface(card.gl.display, card.gl.config, gbmSurface, NULL);
	if (glSurface == EGL_NO_SURFACE) {
		throw errcode_exception(eglGetError(), std::string("failed to create EGL surface. "));
	}

	created = true;
}

uint32_t GBMSurface::getWidth() const {
	return width;
}

uint32_t GBMSurface::getHeight() const {
	return height;
}

uint32_t GBMSurface::getStride() const {
	assert (gbmBO != nullptr);
	return gbm_bo_get_stride(gbmBO);
	
}

uint32_t GBMSurface::getHandle() const {
	assert (gbmBO != nullptr);
	return gbm_bo_get_handle(gbmBO).u32;
	
}


void GBMSurface::destroy() {
	assert(created);
	assert(gbmSurface != nullptr);
	assert(glSurface != EGL_NO_SURFACE);

	eglDestroySurface(card.gl.display, glSurface);
	gbm_surface_destroy(gbmSurface);
	created = false;
}

GBMSurface::~GBMSurface() {
	if (created) {
		destroy();
	}
}
