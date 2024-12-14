#include "DisplayImpl.h"
#include "Displays.h"
#include "ModeEncoder.h"

#include <memory>
#include <purist/platform/exceptions.h>
#include <purist/platform/interfaces.h>

#include <cstdint>
#include <xf86drm.h>

#include <cstring>
#include <cassert>

namespace purist::platform {

static bool modes_equal(const drmModeModeInfo& mode1, const drmModeModeInfo& mode2) {
	return 
		mode1.vdisplay == mode2.vdisplay &&
		mode1.hdisplay == mode2.hdisplay &&
		mode1.clock == mode2.clock;
}

void DisplayImpl::setCrtc(FrameBufferImpl *buf) {
	assert(state != State::CRTC_SET_SUCCESSFULLY);

	if (saved_crtc == nullptr) {
		saved_crtc = std::make_unique<ModeCrtc>(card, crtc_id);
	}

	int ret = drmModeSetCrtc(card.fd, crtc_id, buf->framebuffer_id, 0, 0,
				&connector_id, 1, mode.get());
	
	if (ret) {
		throw errcode_exception(-errno, std::string("cannot assign crtc with framebuffer. ") + strerror(errno));
	}
}



/*
 * modeset_setup_dev() stays the same.
 */

int DisplayImpl::setup(const ModeResources& res, const ModeConnector& conn) {
	int ret;

	/* check if a monitor is connected */
	if (conn.connector->connection != DRM_MODE_CONNECTED) {
		if (state == State::IN_DRAWING_LOOP) {
			fprintf(stderr, "display disconnected from connector %u\n", conn.connector->connector_id);
			return -ENXIO;
		} else {
			return -ENOENT;
		}
	}

	/* check if there is at least one valid mode */
	if (conn.connector->count_modes == 0) {
		fprintf(stderr, "no valid mode for connector %u\n",	conn.connector->connector_id);
		return -EFAULT;
	}

	bool updating_mode = false;

	int mm = 0;
	// for (mm = 0; mm < conn->count_modes; mm++) {
	// 	fprintf(stderr, "probing %ux%u\n", conn->modes[mm].hdisplay, conn->modes[mm].vdisplay);
	// 	if ((conn->modes[mm].hdisplay == 720 && 
	// 	    conn->modes[mm].vdisplay == 480) ||
	// 		(conn->modes[mm].vdisplay == 720 && 
	// 	    conn->modes[mm].hdisplay == 480)) break;
	// }
	// mm = mm % conn->count_modes;
	// fprintf(stderr, "done! %ux%u\n\n", conn->modes[mm].hdisplay, conn->modes[mm].vdisplay);

	drmModeModeInfo& selected_mode = conn.connector->modes[mm];

	auto new_width = selected_mode.hdisplay;
	auto new_height = selected_mode.vdisplay;

	if (mode != nullptr) {
		// This display already has a mode

		if (modes_equal(*mode, selected_mode)) {

			// No mode change here
			return 0;

		} else {
			uint32_t freq1 = mode->clock * 1000.0f / (mode->htotal * mode->vtotal);
			freq1 = (uint32_t)(freq1 * 1000.0f) / 1000.0f;
			uint32_t freq2 = selected_mode.clock * 1000.0f / (selected_mode.htotal * selected_mode.vtotal);
			freq2 = (uint32_t)(freq2 * 1000.0f) / 1000.0f;

			fprintf(stderr, "changing the mode from %ux%u @ %uHz to %ux%u @ %uHz\n", mode->hdisplay, mode->vdisplay, freq1, 
			                                                                                        new_width, new_height, freq2);
			for (auto& fb : framebuffers) {
				fb->removeAndDestroy();
			}

			updating_mode = true;
			// crtc_set_successfully = false;
			// is_in_drawing_loop = false;
			state = State::INITIALIZED;
			mode = nullptr;
		}
	} else {
		fprintf(stderr, "mode for connector %u is %ux%u\n", conn.connector->connector_id, new_width, new_height);
	}

	/* copy the mode information into our device structure and into both
	* buffers */
	mode = std::make_shared<drmModeModeInfo>(selected_mode);

	if (!updating_mode) {
		/* find a crtc for this connector */
		ret = this->connectDisplayToNotOccupiedCrtc(res, conn);
		if (ret) {
			fprintf(stderr, "no valid crtc for connector %u: \n",
				conn.connector->connector_id);
			return ret;
		}
	}

	/* create framebuffer #1 for this CRTC */
	for (auto& fb : framebuffers) {
		fb->createAndAdd(new_width, new_height);
		setCrtc(fb.get());
	}
	state = State::CRTC_SET_SUCCESSFULLY;

	printf("display connected to connector: %d\n", connector_id); fflush(stdout);
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

void DisplayImpl::draw()
{
	auto next_framebuffer_index = (current_framebuffer_index + 1) % framebuffers.size();
	FrameBufferImpl *next_framebuffer = framebuffers[next_framebuffer_index].get();

	auto next_fb_target = std::dynamic_pointer_cast<TargetSurfaceBackface>(next_framebuffer->getTarget());
	next_fb_target->makeCurrent();
	contents->drawIntoBuffer(*next_fb_target.get());
	next_fb_target->swap();

	auto user_data = this;

	int ret = 0;
	if (page_flips_pending < 2) {
		// For some reason the page flip events accumulate themselves excessively.
		// So here we are adding a new one only if there are less than 2 left in the queue.
		ret = drmModePageFlip(card.fd, crtc_id, next_framebuffer->framebuffer_id, DRM_MODE_PAGE_FLIP_EVENT, user_data);
		if (ret) {
			fprintf(stderr, "cannot flip CRTC for connector %u (%d): %m\n", connector_id, errno);
		} else {
			page_flips_pending += 1;
		}
	}

	if (ret == 0) {
		// Switching the buffer
		current_framebuffer_index = next_framebuffer_index;
	}
}

/*
 * modeset_page_flip_event() is a callback-helper for modeset_draw() below.
 * Please see modeset_draw() for more information.
 *
 * Note that this does nothing if the device is currently cleaned up. This
 * allows to wait for outstanding page-flips during cleanup.
 */

void DisplayImpl::modeset_page_flip_event(int fd, unsigned int frame,
				    unsigned int sec, unsigned int usec, void *data)
{
	DisplayImpl *dev = (DisplayImpl*)data;
	if (dev != nullptr) {
		dev->page_flips_pending -= 1;

		if (!dev->destroying_in_progress) {
			dev->draw();
		}
	}
}

void DisplayImpl::updateInDrawingLoop(DisplayContentsFactory& factory) {
	if (state == State::CRTC_SET_SUCCESSFULLY) { //crtc_set_successfully && !is_in_drawing_loop) {
		state = State::IN_DRAWING_LOOP; //is_in_drawing_loop = true;
		printf("display initialized at connector: %d\n", connector_id); fflush(stdout);
		if (contents == nullptr) {
			contents = factory.createDisplayContents(*this);
		}
		draw();
	}
}


int DisplayImpl::connectDisplayToNotOccupiedCrtc(const ModeResources& res, const ModeConnector& conn) {
	/* first try the currently conected encoder+crtc */
	std::unique_ptr<ModeEncoder> enc;
	if (conn.connector->encoder_id)
		enc = std::make_unique<ModeEncoder>(conn);
	else
		enc = nullptr;

	// If we got an encoder...
	if (enc) {
		// ...and the encoder has a CRTC
		if (enc->getCrtcId()) {
			// Check if there is a display that is already connected th this CRTC
			int32_t enc_crtc_id = enc->getCrtcId();
			// for (auto& iter : displays) {
			// 	if (iter->crtc_id == enc_crtc_id) {
			// 		enc_crtc_id = -1;
			// 		break;
			// 	}
			// }
			if (displays.findDisplayConnectedToCrtc(enc_crtc_id) != nullptr) {
				enc_crtc_id = -1;
			}

			// If the display is not found, connecting it to the CRTC
			if (enc_crtc_id >= 0) {
				this->crtc_id = enc_crtc_id;
				return 0;
			}
		}
	}

	/* If the connector is not currently bound to an encoder or if the
	 * encoder+crtc is already used by another connector (actually unlikely
	 * but lets be safe), iterate all other available encoders to find a
	 * matching CRTC. */
	for (unsigned int i = 0; i < conn.connector->count_encoders; ++i) {
		try {
			enc = std::make_unique<ModeEncoder>(conn, i); //drmModeGetEncoder(card.fd, conn->encoders[i]);
		} catch (const errcode_exception& ex) {
			fprintf(stderr, "%s\n", ex.what());
		}

		/* iterate all global CRTCs */
		for (unsigned int j = 0; j < res.getCountCrtcs(); ++j) {
			/* check whether this CRTC works with the encoder */
			if (!(enc->isCrtcPossible(j))) {
				continue;
			}

			/* check that no other device already uses this CRTC */
			int32_t enc_crtc_id = res.getCrtcId(j);
			// for (auto& iter : displays) {
			// 	if (iter->crtc_id == enc_crtc_id) {
			// 		enc_crtc_id = -1;
			// 		break;
			// 	}
			// }
			if (displays.findDisplayConnectedToCrtc(enc_crtc_id) != nullptr) {
				enc_crtc_id = -1;
			}

			/* we have found a CRTC, so save it and return */
			if (enc_crtc_id >= 0) {
				this->crtc_id = enc_crtc_id;
				return 0;
			}
		}

	}

	fprintf(stderr, "cannot find suitable CRTC for connector %u\n", conn.connector->connector_id);
	return -ENOENT;
}

DisplayImpl::~DisplayImpl() {
    drmEventContext ev;

	/* init variables */
	memset(&ev, 0, sizeof(ev));
	ev.version = DRM_EVENT_CONTEXT_VERSION;
	ev.page_flip_handler = DisplayImpl::modeset_page_flip_event;

	destroying_in_progress = true;
	if (page_flips_pending > 0) { fprintf(stderr, "wait for pending page-flip to complete at connector %d...\n", connector_id); }
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
					saved_crtc->crtc->crtc_id,
					saved_crtc->crtc->buffer_id,
					saved_crtc->crtc->x,
					saved_crtc->crtc->y,
					&connector_id,
					1,
					const_cast<drmModeModeInfo*>(&saved_crtc->crtc->mode));

	if (state == State::IN_DRAWING_LOOP) {
		/* destroy framebuffers */
		for (auto& fb : framebuffers) {
			fb->removeAndDestroy();
		}
		printf("display finalized on connector: %d\n", connector_id); fflush(stdout);
	}
}

}