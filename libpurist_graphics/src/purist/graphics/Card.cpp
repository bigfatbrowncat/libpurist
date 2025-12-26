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
#include "Displays.h"
#include "DisplayImpl.h"
#include "ModeResources.h"
#include <purist/exceptions.h>
#include <EGL/egl.h>
#include <cassert>
#include <cstring>
#include <memory>
#include <unistd.h>
#include <fcntl.h>

#include <drm.h>
#include <xf86drm.h>

#include <vector>
#include <iostream>

namespace purist::graphics {

Card::Card(const fs::path& node, bool enableOpenGL) 
	: displays(std::make_shared<Displays>(*this, enableOpenGL)), 
	  fd(-1), enableOpenGL(enableOpenGL), node(node) { }


void Card::initialize(std::shared_ptr<DisplayContentsHandler> contents)
{
	int ret;
	uint64_t has_dumb;

	// Opening the GPU file
	int fd = open(node.c_str(), O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		ret = -errno;
		throw errcode_exception(ret, "cannot open '" + std::string(node) + "'");
	}

	*const_cast<int*>(&this->fd) = fd;
	
	// Probing for DRM
	ModeResources resProbe(*this);

	// Initializing GBM
	if (enableOpenGL) {
		gbmDevice = gbm_create_device(fd);
		if (gbmDevice == nullptr) {
			throw errcode_exception(-errno, "drm device '" + std::string(node) + "' does not support libgbm");
		}
	} else {
		// Configuring dumb buffers ability
		if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb) {
			close(fd);
			throw errcode_exception(-EOPNOTSUPP, "drm device '" + std::string(node) + "' does not support dumb buffers");
		}
	}

	if (enableOpenGL) {
		initGL();
	}

	displays->setDisplayContents(contents);

	/* prepare all connectors and CRTCs */
	ret = displays->updateHardwareConfiguration();
	if (ret) {
		throw errcode_exception(ret, "modeset::prepare failed");
	}
	
	displays->addNewlyConnectedToDrawingLoop();

}



static int match_config_to_visual(EGLDisplay egl_display, EGLint visual_id,
                                  const std::vector<EGLConfig>& configs, int count) {

  EGLint id;
  for (int i = 0; i < count; ++i) {
    if (!eglGetConfigAttrib(egl_display, configs[i], EGL_NATIVE_VISUAL_ID, &id))
      continue;
    if (id == visual_id)
      return i;
  }
  return -1;
}

static void EGLAPIENTRY DebugMessageCallback(EGLenum error,
                                         const char *command,
                                         EGLint messageType,
                                         EGLLabelKHR threadLabel,
                                         EGLLabelKHR objectLabel,
                                         const char *message)
{
	if (messageType == EGL_DEBUG_MSG_CRITICAL_KHR || 
		messageType == EGL_DEBUG_MSG_ERROR_KHR) {

		std::cout << "EGL Debug Message:" << std::endl;
		std::cout << "  command: " << std::hex << command << std::endl;
		std::cout << "  messageType: " << std::hex << messageType << std::endl;
		std::cout << "  threadLabel: " << std::hex << threadLabel << std::endl;
		std::cout << "  objectLabel: " << std::hex << objectLabel << std::endl;
		std::cout << "  Message: " << message << std::endl;
	}
}

// Function to set up the EGL KHR debug extension
void SetupEGLDebug(EGLDisplay display)
{
    // 1. Get the function pointer for eglDebugMessageControlKHR
    PFNEGLDEBUGMESSAGECONTROLKHRPROC eglDebugMessageControlKHR =
        (PFNEGLDEBUGMESSAGECONTROLKHRPROC)eglGetProcAddress("eglDebugMessageControlKHR");

    if (!eglDebugMessageControlKHR) {
        std::cerr << "EGL_KHR_debug extension not available or function pointer not found." << std::endl;
        return;
    }

    // 2. Define the attributes for message control
    // Here we enable all message types (EGL_DEBUG_MSG_CRITICAL_KHR, EGL_DEBUG_MSG_ERROR_KHR,
    // EGL_DEBUG_MSG_WARNING_KHR, EGL_DEBUG_MSG_INFO_KHR).
    // An empty attribute list (EGL_NONE) enables all messages if the filter is set to true.
    EGLAttrib attributes[] = {
        EGL_DEBUG_MSG_CRITICAL_KHR, EGL_TRUE,
        EGL_DEBUG_MSG_ERROR_KHR,    EGL_FALSE,
        EGL_DEBUG_MSG_WARN_KHR,     EGL_FALSE,
        EGL_DEBUG_MSG_INFO_KHR,     EGL_FALSE,
        EGL_NONE
    };

    // 3. Register the callback function and set the message control attributes
    EGLint result = eglDebugMessageControlKHR(
        (EGLDEBUGPROCKHR)&DebugMessageCallback,
        attributes // Pass the filter attributes
    );

    if (result != EGL_SUCCESS) {
        std::cerr << "Failed to set EGL debug message control: " << std::hex << eglGetError() << std::endl;
    } else {
        std::cout << "EGL debug callback successfully set." << std::endl;
    }
}

int Card::initGL()
{
	EGLint config_attribs[] = {
		EGL_SURFACE_TYPE,     EGL_WINDOW_BIT,
        EGL_RED_SIZE,         8,
		EGL_GREEN_SIZE,       8,
		EGL_BLUE_SIZE,        8,
        EGL_ALPHA_SIZE,       0,
        EGL_RENDERABLE_TYPE,  EGL_OPENGL_ES2_BIT,
        EGL_NONE
	};

	EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 	2,
		EGL_NONE
	};

	EGLint major, minor;

	SetupEGLDebug(nullptr);

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		printf("failed to bind api EGL_OPENGL_ES_API\n");
		return -1;
	}


	PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = NULL;
	eglGetPlatformDisplayEXT =
		(PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");
	assert(eglGetPlatformDisplayEXT != NULL);

	glDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, gbmDevice, NULL);
	if (glDisplay == EGL_NO_DISPLAY) {
		printf("eglGetPlatformDisplayEXT() failed\n");
		return -1;
	}

	SetupEGLDebug(glDisplay);

	if (!eglInitialize(glDisplay, &major, &minor)) {
		printf("eglInitialize() failed\n");
		return -1;
	}

	printf("EGL Display %p initialized with with EGL version %d.%d\n", glDisplay, major, minor);

	printf("EGL Version \"%s\"\n", eglQueryString(glDisplay, EGL_VERSION));
	printf("EGL Vendor \"%s\"\n", eglQueryString(glDisplay, EGL_VENDOR));
	printf("EGL Extensions \"%s\"\n", eglQueryString(glDisplay, EGL_EXTENSIONS));


	EGLint count = 0;
	eglGetConfigs(glDisplay, NULL, 0, &count);
	std::vector<EGLConfig> configs(count);
	eglGetConfigs(glDisplay, configs.data(), configs.size(), &count);
	assert(count == (EGLint)configs.size());

	EGLint num_config;
	if (!eglChooseConfig(glDisplay, config_attribs, configs.data(), count, &num_config)) {
		printf("failed to choose config: %d\n", num_config);
		return -1;
	}

	auto config_index = match_config_to_visual(glDisplay, GBM_FORMAT_XRGB8888, configs, num_config);

	glConfig = configs[config_index];

	glContext = eglCreateContext(glDisplay, glConfig, EGL_NO_CONTEXT, context_attribs);
	if (glContext == EGL_NO_CONTEXT) {
		printf("failed to create context\n");
		return -1;
	}

	printf("GL Extensions: \"%s\"\n", glGetString(GL_EXTENSIONS));

	return 0;
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

	if (eglGetCurrentContext() == glContext) {
		// This will ensure that the context is immediately deleted.
		eglMakeCurrent(glDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	}

	if (EGL_NO_CONTEXT != glContext) {
		auto res = eglDestroyContext(glDisplay, glContext);
		if (res != GL_TRUE) {
			std::cout << "eglDestroyContext() returns " << res << std::endl;
		}
		glContext = EGL_NO_CONTEXT;
	}

	if (EGL_NO_DISPLAY != glDisplay) {
		auto res = eglTerminate(glDisplay);
		if (res != GL_TRUE) {
			std::cout << "eglTerminate() returns " << res << std::endl;
		}
		glDisplay = EGL_NO_DISPLAY;
	}

	if (gbmDevice != nullptr) {
		gbm_device_destroy(gbmDevice);
		std::cout << "gbm_device_destroy() done" << std::endl;
	}

    // Closing the video card file
	close(fd);
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

void Card::processFd(std::vector<pollfd>::iterator fds_iter)
{
	drmEventContext ev;

	/* init variables */
	time_t start;
	srand(time(&start));
	//FD_ZERO(&fds);
	//memset(&v, 0, sizeof(v));
	memset(&ev, 0, sizeof(ev));
	/* Set this to only the latest version you support. Version 2
	 * introduced the page_flip_handler, so we use that. */
	ev.version = 2;
	ev.page_flip_handler = DisplayImpl::modeset_page_flip_event; //  unsigned int frame, unsigned int sec, unsigned int usec, void *data


	// We update the displays list each 100 event readings. That has to be frequent enough
	int redraws_between_updates = 100;

    if (fds_iter->fd != fd) throw errcode_exception(-1, "The fd is not the videocard's fd");

    if (fds_iter->revents != 0) {
        drmHandleEvent(fd, &ev);

    }

	if (card_poll_counter % redraws_between_updates == 0) {
		int ret = displays->updateHardwareConfiguration();
		if (ret) {
			throw errcode_exception(ret, "Displays::updateHardwareConfiguration() failed");
		}
		displays->addNewlyConnectedToDrawingLoop();
	}

	card_poll_counter ++;
}

// void Card::runDrawingLoop()
// {
// 	int ret;
// 	fd_set fds;
// 	time_t start, cur;
// 	struct timeval v;
// 	drmEventContext ev;

// 	if (enableOpenGL) {
// 		initGL();
// 	}

// 	/* init variables */
// 	srand(time(&start));
// 	FD_ZERO(&fds);
// 	memset(&v, 0, sizeof(v));
// 	memset(&ev, 0, sizeof(ev));
// 	/* Set this to only the latest version you support. Version 2
// 	 * introduced the page_flip_handler, so we use that. */
// 	ev.version = 2;
// 	ev.page_flip_handler = DisplayImpl::modeset_page_flip_event; //  unsigned int frame, unsigned int sec, unsigned int usec, void *data

// 	//displays->createContentsForAll();

// 	/* prepare all connectors and CRTCs */
// 	ret = displays->updateHardwareConfiguration();
// 	if (ret) {
// 		throw errcode_exception(ret, "modeset::prepare failed");
// 	}
	
// 	displays->addNewlyConnectedToDrawingLoop();

// 	// We update the displays list each 100 event readings. That has to be frequent enough
// 	int counter = 0, redraws_between_updates = 100;

// 	int seconds = 600;
// 	/* wait 5s for VBLANK or input events */
// 	while (time(&cur) < start + seconds) {
// 		FD_SET(0, &fds);
// 		FD_SET(fd, &fds);
// 		v.tv_sec = start + seconds - cur;

// 		ret = select(fd + 1, &fds, NULL, NULL, &v);
// 		if (ret < 0) {
// 			fprintf(stderr, "select() failed with %d: %m\n", errno);
// 			break;
// 		} else if (FD_ISSET(0, &fds)) {
// 			fprintf(stderr, "exit due to user-input\n");
// 			break;
// 		} else if (FD_ISSET(fd, &fds)) {
// 			// process
// 		}
// 	}
// }

// void Card::setDisplayContentsFactory(std::shared_ptr<DisplayContentsFactory> factory) {
// 	displays->setDisplayContentsFactory(factory);
// }


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

}