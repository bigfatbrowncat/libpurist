#include "DumbBufferMapping.h"
#include "DumbBufferTargetSurfaceImpl.h"
#include <purist/platform/exceptions.h>
#include <drm.h>
#include <sys/mman.h>
#include <xf86drm.h>

#include <cstring>

namespace purist::platform {

DumbBufferMapping::DumbBufferMapping(const Card& card, const DumbBufferTargetSurfaceImpl& dumb)
		: card(card), /*buf(buf),*/ dumb(dumb), map((uint8_t*)MAP_FAILED) { }

void DumbBufferMapping::doMapping() {
	struct drm_mode_map_dumb mreq;

	/* prepare buffer for memory mapping */
	memset(&mreq, 0, sizeof(mreq));
	mreq.handle = dumb.getHandle();
	int ret = drmIoctl(this->card.fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
	if (ret) {
		throw errcode_exception(-errno, std::string("cannot map dumb buffer. ") + strerror(errno));
	}

	/* perform actual memory mapping */
	this->map = (uint8_t *)mmap(0, dumb.getSize(), PROT_READ | PROT_WRITE, MAP_SHARED, card.fd, mreq.offset);
	if (this->map == MAP_FAILED) {
		throw errcode_exception(-errno, std::string("cannot call mmap on a dumb buffer. ") + strerror(errno));
	}

}

void DumbBufferMapping::doUnmapping() {
	if (this->map != MAP_FAILED) {
		/* unmap buffer */
		munmap((void*)this->map, dumb.getSize());
	}
}

DumbBufferMapping::~DumbBufferMapping() {
	doUnmapping();
}

}