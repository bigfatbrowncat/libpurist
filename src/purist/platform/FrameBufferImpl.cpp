#include "FrameBufferImpl.h"
#include "DumbBufferTargetSurface.h"
#include "EGLTargetSurface.h"
#include <purist/platform/exceptions.h>
#include <EGL/egl.h>
#include <xf86drmMode.h>

#include <cstring>
#include <cassert>
#include <memory>

std::shared_ptr<TargetSurface> FrameBufferImpl::target_for(bool opengl, const Card& card) {
	if (opengl) {
		return std::make_shared<EGLTargetSurface>(card);
	} else {
		return std::make_shared<DumbBufferTargetSurface>(card);
	}
}

FrameBufferImpl::FrameBufferImpl(const Card& card, bool opengl)
	: card(card), target(target_for(opengl, card)), enableOpenGL(opengl)
{
}

void FrameBufferImpl::createAndAdd(int width, int height) {
	assert(!added);
	target->create(width, height);

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
}

void FrameBufferImpl::removeAndDestroy() {
	assert(added);

	int ret = drmModeRmFB(card.fd, framebuffer_id);
	if (ret) {
		throw errcode_exception(-errno, std::string("cannot destroy framebuffer. ") + strerror(errno));
	}

	// Destroying the target
	target->destroy();

	added = false;
}

FrameBufferImpl::~FrameBufferImpl() {
	if (added) {
		removeAndDestroy();
	}
}

