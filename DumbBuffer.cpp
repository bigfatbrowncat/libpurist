#include "DumbBuffer.h"
#include "exceptions.h"

#include <xf86drm.h>

#include <cassert>
#include <cstring>
#include <cerrno>
#include <string>

DumbBuffer::DumbBuffer(const Card& card) 
		:  card(card), stride(0), size(0), handle(0), width(0), height(0) {
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

	*const_cast<uint32_t*>(&this->stride) = creq.pitch;
	*const_cast<uint32_t*>(&this->size) = creq.size;
	*const_cast<uint32_t*>(&this->handle) = creq.handle;

	*const_cast<int*>(&this->width) = width;
	*const_cast<int*>(&this->height) = height;

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