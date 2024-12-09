#include "FrameBuffer.h"
#include "Display.h"
#include "DumbBuffer.h"
#include "DumbBufferMapping.h"
#include "exceptions.h"

#include <EGL/egl.h>
#include <xf86drmMode.h>

#include <cstring>
#include <cassert>
#include <memory>

FrameBuffer::FrameBuffer(const Card& card, Display& display)
	: card(card), display(display), target(std::make_shared<GBMSurface>(card))//, mapping(std::make_shared<DumbBufferMapping>(card, *dumb))
{}

void FrameBuffer::activate() {
	assert(!active);
	target->activate();
	int ret = drmModeAddFB(card.fd, target->getWidth(), target->getHeight(), 24, 32, target->getStride(),
			target->getHandle(), const_cast<uint32_t*>(&this->framebuffer_id));
	if (ret) {
		throw errcode_exception(-errno, std::string("cannot create framebuffer. ") + strerror(errno));
	}
	display.setCrtc(this);
	active = true;
}

void FrameBuffer::deactivate() {
	assert(active);
   	int ret = drmModeRmFB(card.fd, framebuffer_id);
	if (ret) {
	 	throw errcode_exception(-errno, std::string("cannot destroy framebuffer. ") + strerror(errno));
	}
	target->deactivate();
	active = false;
}


void FrameBuffer::createAndAdd(int width, int height) {
	//dumb->create(width, height);
	//mapping->doMapping();
	target->create(width, height);

	//Card& card = const_cast<Card&>(this->card);
	
	// target->activate();

	// glClearColor(1.0f - 0, 0.5, 0.0, 1.0);
  	// glClear(GL_COLOR_BUFFER_BIT);

	// target->swap();
	
	
	// card.gbm.bo = gbm_surface_lock_front_buffer(card.gbm.surface);
	// card.gbm.handle = gbm_bo_get_handle(card.gbm.bo).u32;
	// card.gbm.pitch = gbm_bo_get_stride(card.gbm.bo);


	// int ret = drmModeAddFB(card.fd, width, height, 24, 32, target->getStride(),
	//  			target->getHandle(), const_cast<uint32_t*>(&this->framebuffer_id));


	/* create framebuffer object for the dumb-buffer */
	// int ret = drmModeAddFB(this->card.fd, 
	//                        this->dumb->width, this->dumb->height, 
	// 					   24, 32, 
	// 					   dumb->stride, dumb->handle, 
	// 					   const_cast<uint32_t*>(&this->framebuffer_id));
	// if (ret) {
	// 	throw errcode_exception(-errno, std::string("cannot create framebuffer. ") + strerror(errno));
	// }
	added = true;

	/* clear the framebuffer to 0 */
	//memset((void*)this->mapping->map, 0, dumb->size);

}

void FrameBuffer::removeAndDestroy() {
	assert(added);

	if (active) { 
		deactivate(); 
	}

	// Unmapping the dumb
	if (0) mapping->doUnmapping();

	// Destroying the target
	target->destroy();

	added = false;
}

FrameBuffer::~FrameBuffer() {
	if (added) {
		removeAndDestroy();
	}
}

