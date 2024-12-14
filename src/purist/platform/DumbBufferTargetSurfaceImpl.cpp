#include "DumbBufferTargetSurfaceImpl.h"

#include <purist/platform/exceptions.h>

#include <EGL/egl.h>
#include <gbm.h>
#include <xf86drm.h>

#include <cassert>
#include <cstring>
#include <cerrno>
#include <string>

namespace purist::platform {

DumbBufferTargetSurfaceImpl::DumbBufferTargetSurfaceImpl(const Card& card) 
	: card(card), stride(0), size(0), handle(0), width(0), height(0), 
	  mapping(std::make_shared<DumbBufferMapping>(card, *this)) {
}

void DumbBufferTargetSurfaceImpl::create(int width, int height) {
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

	mapping->doMapping();

	/* clear the framebuffer to 0 */
	memset((void*)this->mapping->map, 0, this->size);
}

void DumbBufferTargetSurfaceImpl::destroy() {
	assert(created);

	mapping->doUnmapping();

	struct drm_mode_destroy_dumb dreq;
	memset(&dreq, 0, sizeof(dreq));
	dreq.handle = handle;

	int ret = drmIoctl(card.fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
	if (ret < 0) {
		throw errcode_exception(-errno, std::string("cannot destroy dumb buffer. ") + strerror(errno));
	}
	created = false;
}

DumbBufferTargetSurfaceImpl::~DumbBufferTargetSurfaceImpl() {
	if (created) {
		destroy();
	}
}

}