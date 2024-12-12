#include "FrameBuffer.h"
#include "Display.h"
#include "DumbBufferTargetSurface.h"
#include "EGLTargetSurface.h"
#include "exceptions.h"

#include <EGL/egl.h>
#include <xf86drmMode.h>

#include <cstring>
#include <cassert>
#include <memory>

std::shared_ptr<TargetSurface> FrameBuffer::target_for(bool opengl, const Card& card) {
	if (opengl) {
		return std::make_shared<GBMSurface>(card);
	} else {
		return std::make_shared<DumbBufferTargetSurface>(card);
	}
}

FrameBuffer::FrameBuffer(const Card& card, Display& display, bool opengl)
	: card(card), display(display), target(target_for(opengl, card)), enableOpenGL(opengl)
{
}

void FrameBuffer::createAndAdd(int width, int height) {
	assert(!added);
	target->create(width, height);
	//mapping->doMapping();

	target->makeCurrent();
	target->swap();

	target->lock();
	int ret = drmModeAddFB(card.fd, target->getWidth(), target->getHeight(), 24, 32, target->getStride(),
			target->getHandle(), const_cast<uint32_t*>(&this->framebuffer_id));
	if (ret) {
		throw errcode_exception(-errno, std::string("cannot create framebuffer. ") + strerror(errno));
	}
	target->unlock();
	added = true;
	display.setCrtc(this);

	added = true;

}

void FrameBuffer::removeAndDestroy() {
	assert(added);

	int ret = drmModeRmFB(card.fd, framebuffer_id);
	if (ret) {
		throw errcode_exception(-errno, std::string("cannot destroy framebuffer. ") + strerror(errno));
	}

	// Unmapping the dumb
	//mapping->doUnmapping();

	// Destroying the target
	target->destroy();

	added = false;
}

FrameBuffer::~FrameBuffer() {
	if (added) {
		removeAndDestroy();
	}
}

