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
#include "Display.h"
#include "exceptions.h"

#include <cassert>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

#include <drm.h>
#include <xf86drm.h>

#include <vector>

/* Draw code here */
// static void draw_color(uint32_t i)
// {
// 	glClear(GL_COLOR_BUFFER_BIT);
// 	glClearColor(0.2f, 0.3f, 0.5f, 1.0f);
// }


Card::Card(const char *node) : fd(-1), displays(std::make_shared<Displays>(*this))
{
	int ret;
	uint64_t has_dumb;

	// Opening the GPU file
	int fd = open(node, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		ret = -errno;
		throw errcode_exception(ret, "cannot open '" + std::string(node) + "'");
	}
	
	// Configuring dumb buffers ability
	if (enableDumbBuffers) {
		if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb) {
			close(fd);
			throw errcode_exception(-EOPNOTSUPP, "drm device '" + std::string(node) + "' does not support dumb buffers");
		}
	}

	// Initializing GBM
	if (enableOpenGL) {
		gbmDevice = gbm_create_device(fd);
		if (gbmDevice == nullptr) {
			throw errcode_exception(-errno, "drm device '" + std::string(node) + "' does not support libgbm");
		}
	}

	*const_cast<int*>(&this->fd) = fd;
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

int Card::init_gl(void)
{
	EGLint major, minor;//, n;
	//GLuint vertex_shader, fragment_shader;
	//GLint ret;

	EGLint config_attribs[] = {EGL_SURFACE_TYPE,     	EGL_WINDOW_BIT,
                              EGL_RED_SIZE,         8,
                              EGL_GREEN_SIZE,       8,
                              EGL_BLUE_SIZE,        8,
                              EGL_ALPHA_SIZE,       0,
                              EGL_RENDERABLE_TYPE,  EGL_OPENGL_ES2_BIT,
                              EGL_NONE};

	EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                                         EGL_NONE};

	PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = NULL;
	get_platform_display =
		(PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");
	assert(get_platform_display != NULL);

	/*PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC create_platform_window_surface = NULL;
	create_platform_window_surface =
		(PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC) eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT");
	assert(create_platform_window_surface != NULL);
	*/

	gl.display = get_platform_display(EGL_PLATFORM_GBM_KHR, gbmDevice, NULL);
	//gl.display = eglGetDisplay( (EGLNativeDisplayType)gbm.dev );

	if (!eglInitialize(gl.display, &major, &minor)) {
		printf("failed to initialize\n");
		return -1;
	}

	printf("Using display %p with EGL version %d.%d\n",
			gl.display, major, minor);

	printf("EGL Version \"%s\"\n", eglQueryString(gl.display, EGL_VERSION));
	printf("EGL Vendor \"%s\"\n", eglQueryString(gl.display, EGL_VENDOR));
	printf("EGL Extensions \"%s\"\n", eglQueryString(gl.display, EGL_EXTENSIONS));

	if (!eglBindAPI(EGL_OPENGL_ES_API) /*EGL_OPENGL_ES_API)*/) {
		printf("failed to bind api EGL_OPENGL_ES_API\n");
		return -1;
	}

	EGLint count = 0;
	eglGetConfigs(gl.display, NULL, 0, &count);

	//EGLConfig *configs;
	//configs = (EGLConfig*)malloc(count * sizeof(*configs));
	std::vector<EGLConfig> configs(count);

	EGLint num_config;
	if (!eglChooseConfig(gl.display, config_attribs, configs.data(), count, &num_config)) {
		printf("failed to choose config: %d\n", num_config);
		return -1;
	}

	auto config_index = match_config_to_visual(gl.display, GBM_FORMAT_XRGB8888, configs, num_config);

	gl.config = configs[config_index];

	gl.context = eglCreateContext(gl.display, gl.config,
			EGL_NO_CONTEXT, context_attribs);
	if (gl.context == NULL) {
		printf("failed to create context\n");
		return -1;
	}


	//gl.surface = create_platform_window_surface(gl.display, gl.config, gbm.surface, NULL);
	// gl.surface = eglCreateWindowSurface(gl.display, gl.config, gbm.surface, NULL);
	// if (gl.surface == EGL_NO_SURFACE) {
	// 	printf("failed to create egl surface: %d\n", eglGetError());
	// 	return -1;
	// }


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
	displays->clear();

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

void Card::runDrawingLoop()
{
	int ret;
	fd_set fds;
	time_t start, cur;
	struct timeval v;
	drmEventContext ev;

	init_gl();

	/* init variables */
	srand(time(&start));
	FD_ZERO(&fds);
	memset(&v, 0, sizeof(v));
	memset(&ev, 0, sizeof(ev));
	/* Set this to only the latest version you support. Version 2
	 * introduced the page_flip_handler, so we use that. */
	ev.version = 2;
	ev.page_flip_handler = Display::modeset_page_flip_event; //  unsigned int frame, unsigned int sec, unsigned int usec, void *data


	/* prepare all connectors and CRTCs */
	ret = displays->update();
	if (ret)
		throw errcode_exception(ret, "modeset::prepare failed");

	// bool modeset_success = displays->setAllCrtcs();
	// if (!modeset_success)
	// 	throw std::runtime_error("mode setting failed for some displays (1)");

	
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
			printf("drmHandleEvent in Card\n"); fflush(stdout);
			drmHandleEvent(fd, &ev);

			ret = displays->update();
			if (ret)
				throw errcode_exception(ret, "modeset::prepare failed");
			// bool modeset_success = displays->setAllCrtcs();
			// if (!modeset_success)
			// 	throw std::runtime_error("mode setting failed for some displays (2)");

			displays->updateDisplaysInDrawingLoop();

		}
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