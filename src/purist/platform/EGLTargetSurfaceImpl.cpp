#include "EGLTargetSurfaceImpl.h"
#include "Card.h"

#include <purist/platform/exceptions.h>

#include <cassert>
#include <cerrno>

#include <cstring>
#include <string>

EGLTargetSurfaceImpl::EGLTargetSurfaceImpl(const Card& card) 
	: card(card), width(0), height(0) {
}

void EGLTargetSurfaceImpl::create(int width, int height) {
	this->width = width;
	this->height = height;

	gbmSurface = gbm_surface_create(card.getGBMDevice(),
			width, height,
			GBM_FORMAT_XRGB8888,
			GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!gbmSurface) {
		throw errcode_exception(-errno, std::string("failed to create the GBM surface. ") + strerror(errno));
	}

	glSurface = eglCreateWindowSurface(card.glDisplay, card.glConfig, gbmSurface, NULL);
	if (glSurface == EGL_NO_SURFACE) {
		throw errcode_exception(eglGetError(), std::string("failed to create EGL surface. "));
	}

	created = true;
}

uint32_t EGLTargetSurfaceImpl::getWidth() const {
	return width;
}

uint32_t EGLTargetSurfaceImpl::getHeight() const {
	return height;
}

uint32_t EGLTargetSurfaceImpl::getStride() const {
	assert (gbmBO != nullptr);
	return gbm_bo_get_stride(gbmBO);
	
}

uint32_t EGLTargetSurfaceImpl::getHandle() const {
	assert (gbmBO != nullptr);
	return gbm_bo_get_handle(gbmBO).u32;
	
}


void EGLTargetSurfaceImpl::destroy() {
	assert(created);
	assert(gbmSurface != nullptr);
	assert(glSurface != EGL_NO_SURFACE);

	eglDestroySurface(card.glDisplay, glSurface);
	gbm_surface_destroy(gbmSurface);
	created = false;
}

EGLTargetSurfaceImpl::~EGLTargetSurfaceImpl() {
	if (created) {
		destroy();
	}
}

void EGLTargetSurfaceImpl::makeCurrent() {
    eglMakeCurrent(card.glDisplay, glSurface, glSurface, card.glContext);
}

void EGLTargetSurfaceImpl::lock() {
    gbmBO = gbm_surface_lock_front_buffer(gbmSurface);
}

void EGLTargetSurfaceImpl::swap() {
    eglSwapBuffers(card.glDisplay, glSurface);	
}

void EGLTargetSurfaceImpl::unlock() {
    gbm_surface_release_buffer(gbmSurface, gbmBO);
}
