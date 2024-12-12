#include "EGLTargetSurface.h"
#include "Card.h"
#include "exceptions.h"

#include <cassert>
#include <cerrno>

#include <cstring>
#include <string>

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

	glSurface = eglCreateWindowSurface(card.glDisplay, card.glConfig, gbmSurface, NULL);
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

	eglDestroySurface(card.glDisplay, glSurface);
	gbm_surface_destroy(gbmSurface);
	created = false;
}

GBMSurface::~GBMSurface() {
	if (created) {
		destroy();
	}
}

void GBMSurface::makeCurrent() {
    eglMakeCurrent(card.glDisplay, glSurface, glSurface, card.glContext);
}

void GBMSurface::lock() {
    gbmBO = gbm_surface_lock_front_buffer(gbmSurface);
}

void GBMSurface::swap() {
    eglSwapBuffers(card.glDisplay, glSurface);	
}

void GBMSurface::unlock() {
    gbm_surface_release_buffer(gbmSurface, gbmBO);
}
