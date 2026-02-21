#pragma once

#include "purist/exceptions.h"
#include <purist/Platform.h>
#include <purist/graphics/DisplayContentsHandler.h>

#include <memory>
#include <list>
#include <vector>
#include <set>
#include <filesystem>

#include <gbm.h>
#include <poll.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace fs = std::filesystem;

namespace purist::graphics {

class Displays;

class Card : public DeviceClassProvider {
private:
    // Forbidding object copying
    Card(const Card& other) = delete;
    Card& operator = (const Card& other) = delete;

    gbm_device *gbmDevice = nullptr;
    std::shared_ptr<Displays> displays;
    uint32_t card_poll_counter = 0;
    uint32_t redraws_between_updates = 100;

    int initGBM(int fd, uint32_t width, uint32_t height);
    int initGL();


public:
    static std::unique_ptr<Card> probeCard(std::shared_ptr<DisplayContentsHandler> contents, bool enableOpenGL);

    EGLDisplay glDisplay = EGL_NO_DISPLAY;
    EGLConfig glConfig;
    EGLContext glContext = EGL_NO_CONTEXT;
    
    const int fd;
    const bool enableOpenGL = true;
    const fs::path node;

    Card(const fs::path& node, bool enableOpenGL);
    void initialize(std::shared_ptr<DisplayContentsHandler> factory);

    virtual ~Card();

    //void runDrawingLoop();
    void processFd(std::vector<pollfd>::iterator& fds_iter) override;

    gbm_device* getGBMDevice() const { return gbmDevice; }

    std::vector<pollfd> getFds() override { 
        return {
            pollfd {
                .fd = this->fd,
                .events = POLLIN,
                .revents = 0
            }
        };
    }
    void updateHardware() override;

};

}