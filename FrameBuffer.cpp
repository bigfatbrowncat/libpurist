#include "FrameBuffer.h"
#include "DumbBuffer.h"
#include "DumbBufferMapping.h"
#include "exceptions.h"

#include <EGL/egl.h>
#include <xf86drmMode.h>

#include <cstring>
#include <cassert>
#include <memory>

FrameBuffer::FrameBuffer(const Card& card)
	: card(card), dumb(std::make_shared<DumbBuffer>(card)), mapping(std::make_shared<DumbBufferMapping>(card, *dumb))
{}

void FrameBuffer::createAndAdd(int width, int height) {
	//dumb->create(width, height);
	//mapping->doMapping();

	Card& card = const_cast<Card&>(this->card);
	
	if (card.gbm.dev == nullptr) {
		auto ret = card.init_gbm(card.fd, width, height);
		if (ret) {
			printf("failed to initialize GBM\n");
			//return ret;
		}

		ret = card.init_gl();
		if (ret) {
			printf("failed to initialize EGL\n");
			//return ret;
		}

	}

	glClearColor(1.0f - 0, 0.5, 0.0, 1.0);
  	glClear(GL_COLOR_BUFFER_BIT);

	eglSwapBuffers(card.gl.display, card.gl.surface);	
	card.gbm.bo = gbm_surface_lock_front_buffer(card.gbm.surface);
	card.gbm.handle = gbm_bo_get_handle(card.gbm.bo).u32;
	card.gbm.pitch = gbm_bo_get_stride(card.gbm.bo);

	int ret = drmModeAddFB(card.fd, width, height, 24, 32, card.gbm.pitch,
	 			card.gbm.handle, const_cast<uint32_t*>(&this->framebuffer_id));


	/* create framebuffer object for the dumb-buffer */
	// int ret = drmModeAddFB(this->card.fd, 
	//                        this->dumb->width, this->dumb->height, 
	// 					   24, 32, 
	// 					   dumb->stride, dumb->handle, 
	// 					   const_cast<uint32_t*>(&this->framebuffer_id));
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
	gbm_surface_release_buffer(card.gbm.surface, card.gbm.previous_bo);
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

