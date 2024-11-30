/*
 * modeset - DRM Double-Buffered VSync'ed Modesetting Example
 *
 * Written 2012 by David Rheinsberg <david.rheinsberg@gmail.com>
 * Dedicated to the Public Domain.
 */

/*
 * DRM Double-Buffered VSync'ed Modesetting Howto
 * This example extends modeset-double-buffered.c and introduces page-flips
 * synced with vertical-blanks (vsync'ed). A vertical-blank is the time-period
 * when a display-controller pauses from scanning out the framebuffer. After the
 * vertical-blank is over, the framebuffer is again scanned out line by line and
 * followed again by a vertical-blank.
 *
 * Vertical-blanks are important when changing a framebuffer. We already
 * introduced double-buffering, so this example shows how we can flip the
 * buffers during a vertical blank and _not_ during the scanout period.
 *
 * This example assumes that you are familiar with modeset-double-buffered. Only
 * the differences between both files are highlighted here.
 */

#include "Card.h"

#include <cstdint>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>

#include <memory>
#include <cstddef>

#include <cassert>

// The static fields
//std::set<std::shared_ptr<Card::page_flip_callback_data>> Display::page_flip_callback_data_cache;

bool Displays::setAllDisplaysModes() {
	bool some_modeset_failed = false;
	/* perform actual modesetting on each found connector+CRTC */
	for (auto& iter : *this) {
		if (!iter->mode_set_successfully) {
			auto set_successfully = iter->setDisplayMode();
			if (!set_successfully) {
				some_modeset_failed = true;
				fprintf(stderr, "cannot set CRTC for connector %u (%d): %m\n",
					iter->connector_id, errno);
			}
		}
	}
	return !some_modeset_failed;
}

void Displays::updateDisplaysInDrawingLoop() {
	/* redraw all outputs */
	for (auto& iter : *this) {
		if (iter->mode_set_successfully && !iter->is_in_drawing_loop) {
			iter->is_in_drawing_loop = true;
			if (iter->contents == nullptr) {
				iter->contents = this->displayContentsFactory->createDisplayContents();
			}
			iter->draw();
		}
	}
}


int Displays::update()
{
	/* retrieve resources */
	drmModeRes *resources = drmModeGetResources(card.fd);
	if (!resources) {
		throw errcode_exception(-errno, "cannot retrieve DRM resources");
	}


	//std::set<std::shared_ptr<Display>> remaining_displays(this->begin(), this->end());

	/* iterate all connectors */
	for (unsigned int i = 0; i < resources->count_connectors; ++i) {
		/* get information for each connector */
		drmModeConnector *conn = drmModeGetConnector(card.fd, resources->connectors[i]);
		if (!conn) {
			fprintf(stderr, "cannot retrieve DRM connector %u:%u (%d): %m\n",
				i, resources->connectors[i], errno);
			continue;
		}

		std::shared_ptr<Display> display = nullptr;

		// Looking for the display on this connector
		for (auto& disp : *this) {
			if (disp->connector_id == conn->connector_id) {
				display = disp;
			}
		}

		bool new_display_connected = false;
		if (display == nullptr) {
			/* create a device structure */
			display = std::make_shared<Display>(card, *this);
			new_display_connected = true;
			display->connector_id = conn->connector_id;
		}

		/* call helper function to prepare this connector */
		int ret = display->setup(resources, conn);
		if (ret) {
			if (ret == -ENXIO) {
				this->remove(display);
			} else if (ret != -ENOENT) {
				errno = -ret;
				fprintf(stderr, "cannot setup device for connector %u:%u (%d): %m\n",
					i, resources->connectors[i], errno);
			}
			display = nullptr;
			drmModeFreeConnector(conn);
			continue;
		}

		/* free connector data and link device into global list */
		drmModeFreeConnector(conn);
		
		if (new_display_connected) {
			this->push_front(display);
		}
	}

	// if (!remaining_displays.empty()) {
	// 	fprintf(stderr, "%lu displays disconnected\n", remaining_displays.size());

	// 	for (auto& disp : remaining_displays) {
	// 		this->remove(disp);
	// 	}

	// 	remaining_displays.clear();
	// }

	/* free resources again */
	drmModeFreeResources(resources);
	return 0;
}

int Displays::findCrtcForDisplay(drmModeRes *res, drmModeConnector *conn, Display& display) const {
	unsigned int i, j;
	int32_t enc_crtc_id;

	/* first try the currently conected encoder+crtc */
	drmModeEncoder *enc;
	if (conn->encoder_id)
		enc = drmModeGetEncoder(card.fd, conn->encoder_id);
	else
		enc = nullptr;

	if (enc) {
		if (enc->crtc_id) {
			enc_crtc_id = enc->crtc_id;
			for (auto& iter : *this) {
				if (iter->crtc_id == enc_crtc_id) {
					enc_crtc_id = -1;
					break;
				}
			}

			if (enc_crtc_id >= 0) {
				drmModeFreeEncoder(enc);
				display.crtc_id = enc_crtc_id;
				return 0;
			}
		}

		drmModeFreeEncoder(enc);
	}

	/* If the connector is not currently bound to an encoder or if the
	 * encoder+crtc is already used by another connector (actually unlikely
	 * but lets be safe), iterate all other available encoders to find a
	 * matching CRTC. */
	for (i = 0; i < conn->count_encoders; ++i) {
		enc = drmModeGetEncoder(card.fd, conn->encoders[i]);
		if (!enc) {
			fprintf(stderr, "cannot retrieve encoder %u:%u (%d): %m\n",
				i, conn->encoders[i], errno);
			continue;
		}

		/* iterate all global CRTCs */
		for (j = 0; j < res->count_crtcs; ++j) {
			/* check whether this CRTC works with the encoder */
			if (!(enc->possible_crtcs & (1 << j)))
				continue;

			/* check that no other device already uses this CRTC */
			enc_crtc_id = res->crtcs[j];
			for (auto& iter : *this) {
				if (iter->crtc_id == enc_crtc_id) {
					enc_crtc_id = -1;
					break;
				}
			}

			/* we have found a CRTC, so save it and return */
			if (enc_crtc_id >= 0) {
				drmModeFreeEncoder(enc);
				display.crtc_id = enc_crtc_id;
				return 0;
			}
		}

		drmModeFreeEncoder(enc);
	}

	fprintf(stderr, "cannot find suitable CRTC for connector %u\n",
		conn->connector_id);
	return -ENOENT;
}

Display::~Display() {
    drmEventContext ev;

	/* init variables */
	memset(&ev, 0, sizeof(ev));
	ev.version = DRM_EVENT_CONTEXT_VERSION;
	ev.page_flip_handler = Card::modeset_page_flip_event;


	cleanup = true;
	if (pflip_pending) { fprintf(stderr, "wait for pending page-flip to complete...\n"); }
	while (pflip_pending) {
		int ret = drmHandleEvent(card.fd, &ev);
		if (ret) {
			break;
		}
	}

	/* restore saved CRTC configuration */
	if (!pflip_pending && mode_set_successfully)
		drmModeSetCrtc(card.fd,
					saved_crtc->crtc_id,
					saved_crtc->buffer_id,
					saved_crtc->x,
					saved_crtc->y,
					&connector_id,
					1,
					&saved_crtc->mode);
	drmModeFreeCrtc(saved_crtc);

	if (is_in_drawing_loop) {
		/* destroy framebuffers */
		bufs[1].removeAndDestroy();
		bufs[0].removeAndDestroy();
	}
}


Displays::~Displays() {
	clear();
}


Card::Card(const char *node) : fd(-1), displays(std::make_shared<Displays>(*this))
{
	int ret;
	uint64_t has_dumb;

	int fd = open(node, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		ret = -errno;
		throw errcode_exception(ret, "cannot open '" + std::string(node) + "'");
	}

	if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb) {
		close(fd);
		throw errcode_exception(-EOPNOTSUPP, "drm device '" + std::string(node) + "' does not support dumb buffers");
	}

	*const_cast<int*>(&this->fd) = fd;


}


/*
 * modeset_cleanup() stays mostly the same. However, before resetting a CRTC to
 * its previous state, we wait for any outstanding page-flip to complete. This
 * isn't strictly neccessary, however, some DRM drivers are known to be buggy if
 * we call drmModeSetCrtc() if there is a pending page-flip.
 * Furthermore, we don't want any pending page-flips when our application exist.
 * Because another application might pick up the DRM device and try to schedule
 * their own page-flips which might then fail as long as our page-flip is
 * pending.
 * So lets be safe here and simply wait for any page-flips to complete. This is
 * a blocking operation, but it's mostly just <16ms so we can ignore that.
 */
Card::~Card() {
	displays = nullptr;

    // Closing the video card file
	close(fd);
}


bool Display::setDisplayMode() {
	if (saved_crtc == nullptr) {
		saved_crtc = drmModeGetCrtc(card.fd, crtc_id);
	}

	FrameBuffer *buf = &bufs[front_buf];
	int ret = drmModeSetCrtc(card.fd, crtc_id, buf->framebuffer_id, 0, 0,
				&connector_id, 1, mode.get());
	
	if (ret == 0) {
		mode_set_successfully = true;
		is_in_drawing_loop = false;
	} else {
		mode_set_successfully = false;
		is_in_drawing_loop = false;
	}

	return mode_set_successfully;
}


/*
 * modeset_setup_dev() stays the same.
 */

int Display::setup(drmModeRes *res, drmModeConnector *conn) {
	int ret;

	/* check if a monitor is connected */
	if (conn->connection != DRM_MODE_CONNECTED) {
		if (is_in_drawing_loop) {
			fprintf(stderr, "display disconnected from connector %u\n", conn->connector_id);
			return -ENXIO;
		} else {
			return -ENOENT;
		}
	}

	/* check if there is at least one valid mode */
	if (conn->count_modes == 0) {
		fprintf(stderr, "no valid mode for connector %u\n",	conn->connector_id);
		return -EFAULT;
	}

	bool updating_mode = false;

	auto new_width = conn->modes[0].hdisplay;
	auto new_height = conn->modes[0].vdisplay;

	if (mode != nullptr) {
		if (mode->vdisplay == conn->modes[0].vdisplay &&
			mode->hdisplay == conn->modes[0].hdisplay &&
			mode->clock == conn->modes[0].clock) {

			// No mode change here
			return 0;

		} else {
			uint32_t freq1 = mode->clock * 1000.0f / (mode->htotal * mode->vtotal);
			freq1 = (uint32_t)(freq1 * 1000.0f) / 1000.0f;
			uint32_t freq2 = conn->modes[0].clock * 1000.0f / (conn->modes[0].htotal * conn->modes[0].vtotal);
			freq2 = (uint32_t)(freq2 * 1000.0f) / 1000.0f;

			fprintf(stderr, "Changing the mode from %ux%u @ %uHz to %ux%u @ %uHz\n", mode->hdisplay, mode->vdisplay, freq1, 
			                                                                          new_width, new_height, freq2);
			bufs[0].removeAndDestroy();
			bufs[1].removeAndDestroy();
			updating_mode = true;
			mode_set_successfully = false;
			is_in_drawing_loop = false;
			mode = nullptr;
		}
	} else {
		fprintf(stderr, "mode for connector %u is %ux%u\n", conn->connector_id, new_width, new_height);
	}

	/* copy the mode information into our device structure and into both
	* buffers */
	mode = std::make_shared<drmModeModeInfo>(conn->modes[0]);

	if (!updating_mode) {
		/* find a crtc for this connector */
		ret = displays.findCrtcForDisplay(res, conn, *this);
		if (ret) {
			fprintf(stderr, "no valid crtc for connector %u: \n",
				conn->connector_id);
			return ret;
		}
	}

	/* create framebuffer #1 for this CRTC */
	bufs[0].createAndAdd(new_width, new_height);
	bufs[1].createAndAdd(new_width, new_height);

	return 0;
}

/*
 * modeset_find_crtc() stays the same.
 */

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

DumbBufferMapping::DumbBufferMapping(const Card& card, const FrameBuffer& buf, const DumbBuffer& dumb)
		: map((uint8_t*)MAP_FAILED), card(card), buf(buf), dumb(dumb) { }

void DumbBufferMapping::doMapping() {
	struct drm_mode_map_dumb mreq;

	/* prepare buffer for memory mapping */
	memset(&mreq, 0, sizeof(mreq));
	mreq.handle = dumb.handle;
	int ret = drmIoctl(this->card.fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
	if (ret) {
		throw errcode_exception(-errno, std::string("cannot map dumb buffer. ") + strerror(errno));
	}

	/* perform actual memory mapping */
	this->map = (uint8_t *)mmap(0, dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, card.fd, mreq.offset);
	if (this->map == MAP_FAILED) {
		throw errcode_exception(-errno, std::string("cannot call mmap on a dumb buffer. ") + strerror(errno));
	}

}

void DumbBufferMapping::doUnmapping() {
	if (this->map != MAP_FAILED) {
		/* unmap buffer */
		munmap((void*)this->map, dumb.size);
	}
}

DumbBufferMapping::~DumbBufferMapping() {
	doUnmapping();
}

/*
 * modeset_create_fb() stays the same.
 */

FrameBuffer::FrameBuffer(const Card& card)
	: card(card), dumb(nullptr), mapping(nullptr)
{
	*const_cast<std::shared_ptr<DumbBuffer>*>(&this->dumb) = std::make_shared<DumbBuffer>(card);
	*const_cast<std::shared_ptr<DumbBufferMapping>*>(&this->mapping) = std::make_shared<DumbBufferMapping>(card, *this, *dumb);
}

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
	*const_cast<std::shared_ptr<DumbBufferMapping>*>(&this->mapping) = nullptr;
	*const_cast<std::shared_ptr<DumbBuffer>*>(&this->dumb) = nullptr;
}

/*
 * modeset_destroy_fb() stays the same.
 */

// void Card::destroy_fb( struct FrameBuffer *buf)
// {
// 	struct drm_mode_destroy_dumb dreq;

// 	/* unmap buffer */
// 	munmap(buf->map, buf->size);

// 	/* delete framebuffer */
// 	drmModeRmFB(fd, buf->framebuffer_id);

// 	/* delete dumb buffer */
// 	memset(&dreq, 0, sizeof(dreq));
// 	dreq.handle = buf->handle;
// 	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
// }


/*
 * modeset_page_flip_event() is a callback-helper for modeset_draw() below.
 * Please see modeset_draw() for more information.
 *
 * Note that this does nothing if the device is currently cleaned up. This
 * allows to wait for outstanding page-flips during cleanup.
 */

void Card::modeset_page_flip_event(int fd, unsigned int frame,
				    unsigned int sec, unsigned int usec, void *data)
{
	page_flip_callback_data* user_data = reinterpret_cast<page_flip_callback_data*>(data);
	Display *dev = user_data->dev;

	dev->pflip_pending = false;
	if (!dev->cleanup) {
		dev->draw();
	}
}

void Displays::setDisplayContentsFactory(std::shared_ptr<DisplayContentsFactory> factory) {
	this->displayContentsFactory = factory;
}


/*
 * modeset_draw() changes heavily from all previous examples. The rendering has
 * moved into another helper modeset_draw_dev() below, but modeset_draw() is now
 * responsible of controlling when we have to redraw the outputs.
 *
 * So what we do: first redraw all outputs. We initialize the r/g/b/_up
 * variables of each output first, although, you can safely ignore these.
 * They're solely used to compute the next color. Then we call
 * modeset_draw_dev() for each output. This function _always_ redraws the output
 * and schedules a buffer-swap/flip for the next vertical-blank.
 * We now have to wait for each vertical-blank to happen so we can draw the next
 * frame. If a vblank happens, we simply call modeset_draw_dev() again and wait
 * for the next vblank.
 *
 * Note: Different monitors can have different refresh-rates. That means, a
 * vblank event is always assigned to a CRTC. Hence, we get different vblank
 * events for each CRTC/modeset_dev that we use. This also means, that our
 * framerate-controlled color-morphing is different on each monitor. If you want
 * exactly the same frame on all monitors, we would have to share the
 * color-values between all devices. However, for simplicity reasons, we don't
 * do this here.
 *
 * So the last piece missing is how we get vblank events. libdrm provides
 * drmWaitVBlank(), however, we aren't interested in _all_ vblanks, but only in
 * the vblanks for our page-flips. We could use drmWaitVBlank() but there is a
 * more convenient way: drmModePageFlip()
 * drmModePageFlip() schedules a buffer-flip for the next vblank and then
 * notifies us about it. It takes a CRTC-id, fb-id and an arbitrary
 * data-pointer and then schedules the page-flip. This is fully asynchronous and
 * returns immediately.
 * When the page-flip happens, the DRM-fd will become readable and we can call
 * drmHandleEvent(). This will read all vblank/page-flip events and call our
 * modeset_page_flip_event() callback with the data-pointer that we passed to
 * drmModePageFlip(). We simply call modeset_draw_dev() then so the next frame
 * is rendered..
 *
 *
 * So modeset_draw() is reponsible of waiting for the page-flip/vblank events
 * for _all_ currently used output devices and schedule a redraw for them. We
 * could easily do this in a while (1) { drmHandleEvent() } loop, however, this
 * example shows how you can use the DRM-fd to integrate this into your own
 * main-loop. If you aren't familiar with select(), poll() or epoll, please read
 * it up somewhere else. There is plenty of documentation elsewhere on the
 * internet.
 *
 * So what we do is adding the DRM-fd and the keyboard-input-fd (more precisely:
 * the stdin FD) to a select-set and then we wait on this set. If the DRM-fd is
 * readable, we call drmHandleEvents() to handle the page-flip events. If the
 * input-fd is readable, we exit. So on any keyboard input we exit this loop
 * (you need to press RETURN after each keyboard input to make this work).
 */

void Card::runDrawingLoop()
{
	int ret;
	fd_set fds;
	time_t start, cur;
	struct timeval v;
	drmEventContext ev;

	/* init variables */
	srand(time(&start));
	FD_ZERO(&fds);
	memset(&v, 0, sizeof(v));
	memset(&ev, 0, sizeof(ev));
	/* Set this to only the latest version you support. Version 2
	 * introduced the page_flip_handler, so we use that. */
	ev.version = 2;
	
	ev.page_flip_handler = Card::modeset_page_flip_event; //  unsigned int frame, unsigned int sec, unsigned int usec, void *data


	/* prepare all connectors and CRTCs */
	ret = displays->update();
	if (ret)
		throw errcode_exception(ret, "modeset::prepare failed");

	bool modeset_success = displays->setAllDisplaysModes();
	if (!modeset_success)
		throw std::runtime_error("mode setting failed for some displays");

	displays->updateDisplaysInDrawingLoop();

	int seconds = 600;
	/* wait 5s for VBLANK or input events */
	while (time(&cur) < start + seconds) {
		FD_SET(0, &fds);
		FD_SET(fd, &fds);
		v.tv_sec = start + seconds - cur;

		ret = select(fd + 1, &fds, NULL, NULL, &v);
		if (ret < 0) {
			fprintf(stderr, "select() failed with %d: %m\n", errno);
			break;
		} else if (FD_ISSET(0, &fds)) {
			fprintf(stderr, "exit due to user-input\n");
			break;
		} else if (FD_ISSET(fd, &fds)) {
			drmHandleEvent(fd, &ev);

			ret = displays->update();
			if (ret)
				throw errcode_exception(ret, "modeset::prepare failed");
			bool modeset_success = displays->setAllDisplaysModes();
			if (!modeset_success)
				throw std::runtime_error("mode setting failed for some displays");

			displays->updateDisplaysInDrawingLoop();

		}
	}
}


/*
 * modeset_draw_dev() is a new function that redraws the screen of a single
 * output. It takes the DRM-fd and the output devices as arguments, redraws a
 * new frame and schedules the page-flip for the next vsync.
 *
 * This function does the same as modeset_draw() did in the previous examples
 * but only for a single output device now.
 * After we are done rendering a frame, we have to swap the buffers. Instead of
 * calling drmModeSetCrtc() as we did previously, we now want to schedule this
 * page-flip for the next vertical-blank (vblank). We use drmModePageFlip() for
 * this. It takes the CRTC-id and FB-id and will asynchronously swap the buffers
 * when the next vblank occurs. Note that this is done by the kernel, so neither
 * a thread is started nor any other magic is done in libdrm.
 * The DRM_MODE_PAGE_FLIP_EVENT flag tells drmModePageFlip() to send us a
 * page-flip event on the DRM-fd when the page-flip happened. The last argument
 * is a data-pointer that is returned with this event.
 * If we wouldn't pass this flag, we would not get notified when the page-flip
 * happened.
 *
 * Note: If you called drmModePageFlip() and directly call it again, it will
 * return EBUSY if the page-flip hasn't happened in between. So you almost
 * always want to pass DRM_MODE_PAGE_FLIP_EVENT to get notified when the
 * page-flip happens so you know when to render the next frame.
 * If you scheduled a page-flip but call drmModeSetCrtc() before the next
 * vblank, then the scheduled page-flip will become a no-op. However, you will
 * still get notified when it happens and you still cannot call
 * drmModePageFlip() again until it finished. So to sum it up: there is no way
 * to effectively cancel a page-flip.
 *
 * If you wonder why drmModePageFlip() takes fewer arguments than
 * drmModeSetCrtc(), then you should take into account, that drmModePageFlip()
 * reuses the arguments from drmModeSetCrtc(). So things like connector-ids,
 * x/y-offsets and so on have to be set via drmModeSetCrtc() first before you
 * can use drmModePageFlip()! We do this in main() as all the previous examples
 * did, too.
 */

void Display::draw()
{
	FrameBuffer *buf = &bufs[front_buf ^ 1];

	contents->drawIntoBuffer(buf);

	auto user_data = std::make_shared<Card::page_flip_callback_data>(&card, this);
	page_flip_callback_data_cache.insert(user_data);

	int ret = drmModePageFlip(card.fd, crtc_id, buf->framebuffer_id, DRM_MODE_PAGE_FLIP_EVENT, user_data.get());
	if (ret) {
		fprintf(stderr, "cannot flip CRTC for connector %u (%d): %m\n", connector_id, errno);
	} else {
		front_buf ^= 1;
		pflip_pending = true;
	}
}


/*
 * This example shows how to make the kernel handle page-flips and how to wait
 * for them in user-space. The select() example here should show you how you can
 * integrate these loops into your own applications without the need for a
 * separate modesetting thread.
 *
 * However, please note that vsync'ed double-buffering doesn't solve all
 * problems. Imagine that you cannot render a frame fast enough to satisfy all
 * vertical-blanks. In this situation, you don't want to wait after scheduling a
 * page-flip until the vblank happens to draw the next frame. A solution for
 * this is triple-buffering. It should be farily easy to extend this example to
 * use triple-buffering, but feel free to contact me if you have any questions
 * about it.
 * Also note that the DRM kernel API is quite limited if you want to reschedule
 * page-flips that haven't happened, yet. You cannot call drmModePageFlip()
 * twice in a single scanout-period. The behavior of drmModeSetCrtc() while a
 * page-flip is pending might also be unexpected.
 * Unfortunately, there is no ultimate solution to all modesetting problems.
 * This example shows the tools to do vsync'ed page-flips, however, it depends
 * on your use-case how you have to implement it.
 *
 * If you want more code, I can recommend reading the source-code of:
 *  - plymouth (which uses dumb-buffers like this example; very easy to understand)
 *  - kmscon (which uses libuterm to do this)
 *  - wayland (very sophisticated DRM renderer; hard to understand fully as it
 *             uses more complicated techniques like DRM planes)
 *  - xserver (very hard to understand as it is split across many files/projects)
 *
 * Any feedback is welcome. Feel free to use this code freely for your own
 * documentation or projects.
 *
 *  - Hosted on http://github.com/dvdhrm/docs
 *  - Written by David Rheinsberg <david.rheinsberg@gmail.com>
 */