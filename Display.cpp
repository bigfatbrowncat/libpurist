#include "Display.h"
#include "Displays.h"
#include "exceptions.h"
#include "interfaces.h"

#include <cstdint>
#include <xf86drm.h>

#include <cstring>
#include <cassert>


static bool modes_equal(const drmModeModeInfo& mode1, const drmModeModeInfo& mode2) {
	return 
		mode1.vdisplay == mode2.vdisplay &&
		mode1.hdisplay == mode2.hdisplay &&
		mode1.clock == mode2.clock;
}

bool Display::setCrtc(FrameBuffer *buf) {
	if (saved_crtc == nullptr) {
		saved_crtc = drmModeGetCrtc(card.fd, crtc_id);
	}

	//FrameBuffer *buf = &bufs[front_buf];
	int ret = drmModeSetCrtc(card.fd, crtc_id, buf->framebuffer_id, 0, 0,
				&connector_id, 1, mode.get());
	
	if (ret == 0) {
		crtc_set_successfully = true;
		is_in_drawing_loop = false;
	} else {
		crtc_set_successfully = false;
		is_in_drawing_loop = false;
	}

	return crtc_set_successfully;
}


/*
 * modeset_setup_dev() stays the same.
 */

int Display::setup(const drmModeRes *res, const drmModeConnector *conn) {
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
		// This display already has a mode

		if (modes_equal(*mode, conn->modes[0])) {

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
			crtc_set_successfully = false;
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
		ret = this->connectDisplayToNotOccupiedCrtc(res, conn);
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
	//FrameBuffer *fbuf = &bufs[front_buf];
	FrameBuffer *buf = &bufs[front_buf ^ 1];

	buf->target->makeCurrent();
	contents->drawIntoBuffer(buf);
	buf->target->swap();
	// buf->activate();
	// if (fbuf->isActive()) { 
	// 	fbuf->deactivate();
	// }

	auto user_data = this;

	int ret = 0;
	if (page_flips_pending < 2) {
		// For some reason the page flip events accumulating themselves exceedingly.
		// So here we are adding a new one only if there are less than 2 left in the queue.
		ret = drmModePageFlip(card.fd, crtc_id, buf->framebuffer_id, DRM_MODE_PAGE_FLIP_EVENT, user_data);
		if (ret) {
			fprintf(stderr, "cannot flip CRTC for connector %u (%d): %m\n", connector_id, errno);
		} else {
			page_flips_pending += 1;
			// printf("[%lx] page_flip_pending = %d;\n", (uintptr_t)this, page_flips_pending); fflush(stdout);
		}
	}

	if (ret == 0) {
		// Switching the buffer
		front_buf ^= 1;
	}
}

/*
 * modeset_page_flip_event() is a callback-helper for modeset_draw() below.
 * Please see modeset_draw() for more information.
 *
 * Note that this does nothing if the device is currently cleaned up. This
 * allows to wait for outstanding page-flips during cleanup.
 */

void Display::modeset_page_flip_event(int fd, unsigned int frame,
				    unsigned int sec, unsigned int usec, void *data)
{
	Display *dev = (Display*)data;
	if (dev != nullptr) {
		dev->page_flips_pending -= 1;
		// printf("[%lx] dev->page_flip_pending = %d;\n", (uint64_t)dev, dev->page_flips_pending); fflush(stdout);

		if (!dev->destroying_in_progress) {
			dev->draw();
			//dev->swap_buffers();
		}
	}
}

void Display::updateInDrawingLoop(DisplayContentsFactory& factory) {
	if (crtc_set_successfully && !is_in_drawing_loop) {
		is_in_drawing_loop = true;
		if (contents == nullptr) {
			contents = factory.createDisplayContents();
		}
		draw();
	}
}


int Display::connectDisplayToNotOccupiedCrtc(const drmModeRes *res, const drmModeConnector *conn) {
	unsigned int i, j;
	int32_t enc_crtc_id;

	/* first try the currently conected encoder+crtc */
	drmModeEncoder *enc;
	if (conn->encoder_id)
		enc = drmModeGetEncoder(card.fd, conn->encoder_id);
	else
		enc = nullptr;

	// If we got an encoder...
	if (enc) {
		// ...and the encoder has a CRTC
		if (enc->crtc_id) {
			// Check if there is a display that is already connected th this CRTC
			enc_crtc_id = enc->crtc_id;
			for (auto& iter : displays) {
				if (iter->crtc_id == enc_crtc_id) {
					enc_crtc_id = -1;
					break;
				}
			}

			// If the display is not found, connecting it to the CRTC
			if (enc_crtc_id >= 0) {
				drmModeFreeEncoder(enc);
				this->crtc_id = enc_crtc_id;
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
			for (auto& iter : displays) {
				if (iter->crtc_id == enc_crtc_id) {
					enc_crtc_id = -1;
					break;
				}
			}

			/* we have found a CRTC, so save it and return */
			if (enc_crtc_id >= 0) {
				drmModeFreeEncoder(enc);
				this->crtc_id = enc_crtc_id;
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
	ev.page_flip_handler = Display::modeset_page_flip_event;

	destroying_in_progress = true;
	if (page_flips_pending > 0) { fprintf(stderr, "wait for pending page-flip to complete...\n"); }
	while (page_flips_pending > 0) {
		//printf("drmHandleEvent in ~Display\n"); fflush(stdout);
		int ret = drmHandleEvent(card.fd, &ev);
		if (ret) {
			printf("Oopsie!!! %d\n", ret); fflush(stdout);
			break;
		}
	}
	assert(page_flips_pending == 0);

	/* restore saved CRTC configuration */
	if (saved_crtc != nullptr) //crtc_set_successfully)
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

// void Display::swap_buffers() {
// 	Card& card = const_cast<Card&>(this->card);

// 	if (card.gl.display != nullptr) {
// 		eglSwapBuffers(card.gl.display, card.gl.surface);
		
		
// 	//	card.gbm.bo = gbm_surface_lock_front_buffer(card.gbm.surface);
// 	//	card.gbm.handle = gbm_bo_get_handle(card.gbm.bo).u32;
// 	//	card.gbm.pitch = gbm_bo_get_stride(card.gbm.bo);
// 	}

// 	//drmModeAddFB(card.fd, mode->hdisplay, mode->vdisplay, 24, 32, card.gbm.pitch,
// //	 			card.gbm.handle, &card.fb);
// 	// drmModeSetCrtc(device, crtc->crtc_id, fb, 0, 0, &connector_id, 1, &mode_info);
// //	if (card.gbm.previous_bo) {
// //	 	drmModeRmFB(card.fd, card.gbm.previous_fb);
// //	 	gbm_surface_release_buffer(card.gbm.surface, card.gbm.previous_bo);
// //	}
// //	card.gbm.previous_bo = card.gbm.bo;
// //	card.gbm.previous_fb = card.gbm.fb;
// }