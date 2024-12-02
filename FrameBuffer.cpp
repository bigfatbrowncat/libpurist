#include "FrameBuffer.h"
#include "DumbBuffer.h"
#include "DumbBufferMapping.h"
#include "exceptions.h"

#include <xf86drmMode.h>

#include <cstring>
#include <cassert>
#include <memory>

FrameBuffer::FrameBuffer(const Card& card)
	: card(card), dumb(std::make_shared<DumbBuffer>(card)), mapping(std::make_shared<DumbBufferMapping>(card, *dumb))
{}

void FrameBuffer::createAndAdd(int width, int height) {
	dumb->create(width, height);
	mapping->doMapping();

	/* create framebuffer object for the dumb-buffer */
	int ret = drmModeAddFB(this->card.fd, 
	                       this->dumb->width, this->dumb->height, 
						   24, 32, 
						   dumb->stride, dumb->handle, 
						   const_cast<uint32_t*>(&this->framebuffer_id));
	if (ret) {
		throw errcode_exception(-errno, std::string("cannot create framebuffer. ") + strerror(errno));
	}
	added = true;

	/* clear the framebuffer to 0 */
	memset((void*)this->mapping->map, 0, dumb->size);

}

void FrameBuffer::removeAndDestroy() {
	assert(added);

	// Removing the FB
	int ret = drmModeRmFB(card.fd, framebuffer_id);
	if (ret) {
		throw errcode_exception(-errno, std::string("cannot destroy framebuffer. ") + strerror(errno));
	}

	// Unmapping the dumb
	mapping->doUnmapping();

	// Destroying the dumb
	dumb->destroy();

	added = false;
}

FrameBuffer::~FrameBuffer() {
	if (added) {
		removeAndDestroy();
	}
}
